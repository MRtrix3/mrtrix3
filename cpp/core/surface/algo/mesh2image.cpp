/* Copyright (c) 2008-2026 the MRtrix3 contributors.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Covered Software is provided under this License on an "as is"
 * basis, without warranty of any kind, either expressed, implied, or
 * statutory, including, without limitation, warranties that the
 * Covered Software is free of defects, merchantable, fit for a
 * particular purpose or non-infringing.
 * See the Mozilla Public License v. 2.0 for more details.
 *
 * For more details, see http://www.mrtrix.org/.
 */

#include "surface/algo/mesh2image.h"

#include <algorithm>
#include <cmath>
#include <map>
#include <optional>
#include <set>
#include <stack>

#include "header.h"
#include "progressbar.h"
#include "thread_queue.h"
#include "types.h"

#include "surface/filter/vertex_transform.h"
#include "surface/types.h"
#include "surface/utils.h"

namespace MR::Surface::Algo {

constexpr size_t pve_os_ratio = 10;
constexpr size_t pve_nsamples = Math::pow3(pve_os_ratio);

// For every edge voxel, stores those polygons that may intersect the voxel
using Vox2Poly = std::map<Vox, std::vector<size_t>>;

//
// ===================== Toblerone geometric voxelisation primitives =====================
//

namespace {

// Encoding of the per-voxel interior/exterior classification used by the Toblerone algorithm
enum vox_class_t : uint8_t { VC_OUTSIDE = 0, VC_INSIDE = 1, VC_ON_MESH = 2 };

// Default target sub-voxel edge length (mm); the lower limit of feature size found in the brain
constexpr default_type default_subvoxel_mm = 0.75;
// Secondary subdivision factor applied to surface-straddling sub-voxels
constexpr int toblerone_resample_factor = 5;

// Sub-voxel offsets applied to the (y, z) origin of an x-aligned ray, so that the ray does not
//   exactly graze shared polygon edges / vertices; small enough not to alter which sub-region of
//   a voxel a query point falls within
constexpr default_type ray_jitter_y = 7.3e-4;
constexpr default_type ray_jitter_z = 3.1e-4;

// Intersection of an x-aligned ray (origin o, direction +x) with triangle (a, b, c) via the
//   Moller-Trumbore algorithm specialised to a unit x direction.
// \return the signed distance along +x from the origin to the intersection (i.e. intersection.x
//   minus o.x), or std::nullopt if the ray misses or is parallel to the triangle plane.
inline std::optional<default_type> ray_x_triangle(const Vertex &o, const Vertex &a, const Vertex &b, const Vertex &c) {
  constexpr default_type eps = 1e-12;
  const Vertex e1(b - a);
  const Vertex e2(c - a);
  // pvec = dir x e2 with dir = (1, 0, 0) is (0, -e2[2], e2[1])
  const default_type det = -e1[1] * e2[2] + e1[2] * e2[1];
  if (std::fabs(det) < eps)
    return std::nullopt;
  const default_type inv_det = 1.0 / det;
  const Vertex tvec(o - a);
  const default_type u = (-tvec[1] * e2[2] + tvec[2] * e2[1]) * inv_det;
  if (u < 0.0 || u > 1.0)
    return std::nullopt;
  const Vertex qvec(tvec.cross(e1));
  // v = dir . qvec with dir = (1, 0, 0) is qvec[0]
  const default_type v = qvec[0] * inv_det;
  if (v < 0.0 || u + v > 1.0)
    return std::nullopt;
  return e2.dot(qvec) * inv_det;
}

// Count crossings of the +x ray from ray_origin with the local patch `polys`, restricted to
//   intersection x-coordinates within the half-open interval (x_low, x_high].
size_t count_x_crossings(const Vertex &ray_origin,
                         const std::vector<size_t> &polys,
                         const default_type x_low,
                         const default_type x_high,
                         const Mesh &mesh) {
  size_t count = 0;
  VertexList v;
  for (const size_t poly_index : polys) {
    if (poly_index < mesh.num_triangles()) {
      mesh.load_triangle_vertices(v, poly_index);
      const std::optional<default_type> t = ray_x_triangle(ray_origin, v[0], v[1], v[2]);
      if (t) {
        const default_type x = ray_origin[0] + *t;
        if (x > x_low && x <= x_high)
          ++count;
      }
    } else {
      mesh.load_quad_vertices(v, poly_index - mesh.num_triangles());
      const std::optional<default_type> t0 = ray_x_triangle(ray_origin, v[0], v[1], v[2]);
      if (t0) {
        const default_type x = ray_origin[0] + *t0;
        if (x > x_low && x <= x_high)
          ++count;
      }
      const std::optional<default_type> t1 = ray_x_triangle(ray_origin, v[0], v[2], v[3]);
      if (t1) {
        const default_type x = ray_origin[0] + *t1;
        if (x > x_low && x <= x_high)
          ++count;
      }
    }
  }
  return count;
}

// Classify an arbitrary point p (lying within surface-intersected voxel start_vox) as interior or
//   exterior, by casting a +x ray and walking voxels until one of known (non-ON_MESH) class is
//   reached, accumulating surface crossings of each intervening local patch; the parity of the
//   crossing count relative to that anchor classifies p. Beyond the field of view the closed
//   surface is exterior.
vox_class_t classify_point(
    const Vertex &p, const Vox &start_vox, const Vox2Poly &voxel2poly, Image<uint8_t> &voxel_class, const Mesh &mesh) {
  const Vertex ray_origin(p[0], p[1] + ray_jitter_y, p[2] + ray_jitter_z);
  const int size0 = static_cast<int>(voxel_class.size(0));
  size_t crossings = 0;
  Vox v(start_vox);
  // Starting voxel: crossings strictly beyond p along +x, within the voxel's x extent
  {
    const Vox2Poly::const_iterator it = voxel2poly.find(v);
    if (it != voxel2poly.end())
      crossings += count_x_crossings(ray_origin, it->second, p[0], v[0] + 0.5, mesh);
  }
  for (++v[0]; v[0] < size0; ++v[0]) {
    assign_pos_of(v).to(voxel_class);
    const uint8_t cls = voxel_class.value();
    if (cls != VC_ON_MESH)
      return ((crossings & 1U) == 0U) ? static_cast<vox_class_t>(cls) : static_cast<vox_class_t>(cls ^ 1U);
    const Vox2Poly::const_iterator it = voxel2poly.find(v);
    if (it != voxel2poly.end())
      crossings += count_x_crossings(ray_origin, it->second, v[0] - 0.5, v[0] + 0.5, mesh);
  }
  return ((crossings & 1U) == 0U) ? VC_OUTSIDE : VC_INSIDE;
}

// Separating Axis Theorem overlap test between an axis-aligned box (centre, half-extents) and a
//   triangle, after Akenine-Moller.
bool box_triangle_overlap(
    const Vertex &box_centre, const Vertex &box_half, const Vertex &a, const Vertex &b, const Vertex &c) {
  const Vertex v0(a - box_centre);
  const Vertex v1(b - box_centre);
  const Vertex v2(c - box_centre);
  const Vertex e0(v1 - v0);
  const Vertex e1(v2 - v1);
  const Vertex e2(v0 - v2);

  auto separating = [&](const Vertex &axis) -> bool {
    const default_type p0 = axis.dot(v0);
    const default_type p1 = axis.dot(v1);
    const default_type p2 = axis.dot(v2);
    const default_type r =
        box_half[0] * std::fabs(axis[0]) + box_half[1] * std::fabs(axis[1]) + box_half[2] * std::fabs(axis[2]);
    return (std::min({p0, p1, p2}) > r || std::max({p0, p1, p2}) < -r);
  };

  const std::array<Vertex, 3> edges = {e0, e1, e2};
  for (size_t i = 0; i != 3; ++i) {
    Vertex unit(0.0, 0.0, 0.0);
    unit[i] = 1.0;
    for (const Vertex &edge : edges) {
      if (separating(unit.cross(edge)))
        return false;
    }
  }
  for (size_t axis = 0; axis != 3; ++axis) {
    if (std::min({v0[axis], v1[axis], v2[axis]}) > box_half[axis] ||
        std::max({v0[axis], v1[axis], v2[axis]}) < -box_half[axis])
      return false;
  }
  if (separating(e0.cross(e1)))
    return false;
  return true;
}

// Whether a single polygon overlaps the axis-aligned box (centre, half-extents)
bool polygon_box_overlap(const Vertex &centre, const Vertex &half, const size_t poly_index, const Mesh &mesh) {
  VertexList v;
  if (poly_index < mesh.num_triangles()) {
    mesh.load_triangle_vertices(v, poly_index);
    return box_triangle_overlap(centre, half, v[0], v[1], v[2]);
  }
  mesh.load_quad_vertices(v, poly_index - mesh.num_triangles());
  return box_triangle_overlap(centre, half, v[0], v[1], v[2]) || box_triangle_overlap(centre, half, v[0], v[2], v[3]);
}

// Whether any polygon of the local patch overlaps the axis-aligned box (centre, half-extents)
bool patch_overlaps_box(const Vertex &centre, const Vertex &half, const std::vector<size_t> &polys, const Mesh &mesh) {
  for (const size_t poly_index : polys) {
    if (polygon_box_overlap(centre, half, poly_index, mesh))
      return true;
  }
  return false;
}

// General Moller-Trumbore ray-triangle intersection; returns the ray parameter t such that the
//   intersection point is (o + t*d), or std::nullopt if the ray misses or is parallel.
inline std::optional<default_type>
ray_triangle(const Vertex &o, const Vertex &d, const Vertex &a, const Vertex &b, const Vertex &c) {
  constexpr default_type eps = 1e-12;
  const Vertex e1(b - a);
  const Vertex e2(c - a);
  const Vertex pvec(d.cross(e2));
  const default_type det = e1.dot(pvec);
  if (std::fabs(det) < eps)
    return std::nullopt;
  const default_type inv_det = 1.0 / det;
  const Vertex tvec(o - a);
  const default_type u = tvec.dot(pvec) * inv_det;
  if (u < 0.0 || u > 1.0)
    return std::nullopt;
  const Vertex qvec(tvec.cross(e1));
  const default_type v = d.dot(qvec) * inv_det;
  if (v < 0.0 || u + v > 1.0)
    return std::nullopt;
  return e2.dot(qvec) * inv_det;
}

// Append the intersection points of segment [pa, pb] with the local patch polygons
void segment_patch_intersections(
    const Vertex &pa, const Vertex &pb, const std::vector<size_t> &polys, const Mesh &mesh, std::vector<Vertex> &out) {
  const Vertex d(pb - pa);
  VertexList v;
  for (const size_t poly_index : polys) {
    if (poly_index < mesh.num_triangles()) {
      mesh.load_triangle_vertices(v, poly_index);
      const std::optional<default_type> t = ray_triangle(pa, d, v[0], v[1], v[2]);
      if (t && *t >= 0.0 && *t <= 1.0)
        out.push_back(Vertex(pa + (*t) * d));
    } else {
      mesh.load_quad_vertices(v, poly_index - mesh.num_triangles());
      const std::optional<default_type> t0 = ray_triangle(pa, d, v[0], v[1], v[2]);
      if (t0 && *t0 >= 0.0 && *t0 <= 1.0)
        out.push_back(Vertex(pa + (*t0) * d));
      const std::optional<default_type> t1 = ray_triangle(pa, d, v[0], v[2], v[3]);
      if (t1 && *t1 >= 0.0 && *t1 <= 1.0)
        out.push_back(Vertex(pa + (*t1) * d));
    }
  }
}

inline default_type tetra_volume(const Vertex &a, const Vertex &b, const Vertex &c, const Vertex &d) {
  return std::fabs((b - a).dot((c - a).cross(d - a))) / 6.0;
}

// Volume of the part of tetrahedron `v` interior to the plane through q with outward normal n,
//   i.e. the side where (x - q).n <= 0, by enumeration of the marching-tetrahedra cases.
default_type tetra_clip_interior_volume(const std::array<Vertex, 4> &v, const Vertex &q, const Vertex &n) {
  std::array<default_type, 4> f;
  std::array<int, 4> in_idx;
  std::array<int, 4> out_idx;
  size_t ni = 0;
  size_t no = 0;
  for (int i = 0; i != 4; ++i) {
    f[i] = (v[i] - q).dot(n);
    if (f[i] <= 0.0)
      in_idx[ni++] = i;
    else
      out_idx[no++] = i;
  }
  if (ni == 0)
    return 0.0;
  const default_type v_full = tetra_volume(v[0], v[1], v[2], v[3]);
  if (ni == 4)
    return v_full;
  auto cut = [&](const int i, const int j) -> Vertex {
    const default_type t = f[i] / (f[i] - f[j]);
    return Vertex(v[i] + t * (v[j] - v[i]));
  };
  if (ni == 1) {
    const int i = in_idx[0];
    return tetra_volume(v[i], cut(i, out_idx[0]), cut(i, out_idx[1]), cut(i, out_idx[2]));
  }
  if (ni == 3) {
    const int o = out_idx[0];
    return v_full - tetra_volume(v[o], cut(o, in_idx[0]), cut(o, in_idx[1]), cut(o, in_idx[2]));
  }
  // ni == 2: the interior part is a triangular prism (wedge); integrate as a tetra fan from its
  //   centroid over its boundary triangulation (robust as the centroid is strictly interior)
  const int i0 = in_idx[0];
  const int i1 = in_idx[1];
  const int o0 = out_idx[0];
  const int o1 = out_idx[1];
  const std::array<Vertex, 6> w = {v[i0], v[i1], cut(i0, o0), cut(i0, o1), cut(i1, o0), cut(i1, o1)};
  Vertex cen(0.0, 0.0, 0.0);
  for (const Vertex &p : w)
    cen += p;
  cen /= 6.0;
  static const std::array<std::array<int, 3>, 8> tris = {
      {{0, 2, 3}, {1, 5, 4}, {0, 1, 5}, {0, 5, 3}, {0, 1, 4}, {0, 4, 2}, {2, 3, 5}, {2, 5, 4}}};
  default_type vol = 0.0;
  for (const auto &tri : tris)
    vol += tetra_volume(cen, w[tri[0]], w[tri[1]], w[tri[2]]);
  return vol;
}

// Volume of the part of the axis-aligned box (centre, half-extents) interior to the plane through
//   q with outward normal n, via Freudenthal decomposition of the box into six tetrahedra.
default_type box_plane_interior_volume(const Vertex &centre, const Vertex &half, const Vertex &q, const Vertex &n) {
  std::array<Vertex, 8> c;
  for (int i = 0; i != 8; ++i)
    c[i] = Vertex(centre[0] + ((i & 1) ? half[0] : -half[0]),
                  centre[1] + ((i & 2) ? half[1] : -half[1]),
                  centre[2] + ((i & 4) ? half[2] : -half[2]));
  static const std::array<std::array<int, 4>, 6> tets = {
      {{0, 1, 3, 7}, {0, 1, 5, 7}, {0, 2, 3, 7}, {0, 2, 6, 7}, {0, 4, 5, 7}, {0, 4, 6, 7}}};
  default_type vol = 0.0;
  for (const auto &t : tets)
    vol += tetra_clip_interior_volume({c[t[0]], c[t[1]], c[t[2]], c[t[3]]}, q, n);
  return vol;
}

} // namespace

//
// ============================== Shared queue functors ==============================
//

// Streams the set of surface-intersected voxels (and their candidate polygons) into the
//   multi-threaded partial-volume-fraction computation; shared by all algorithm variants
class Source {
public:
  Source(const Vox2Poly &data) : data(data), i(data.begin()) {}

  bool operator()(std::pair<Vox, std::vector<size_t>> &out) {
    if (i == data.end())
      return false;
    out = std::make_pair(i->first, i->second);
    ++i;
    return true;
  }

private:
  const Vox2Poly &data;
  Vox2Poly::const_iterator i;
};

// Brute-force per-voxel partial volume estimator:
//   supersample a dense fixed point lattice and classify each point against the local patch
class BruteForcePipe {
public:
  BruteForcePipe(const Mesh &mesh, const std::vector<Eigen::Vector3d> &polygon_normals)
      : mesh(mesh),
        polygon_normals(polygon_normals)

  {
    // Generate a set of points within this voxel that need to be tested individually
    offsets_to_test.reset(new std::vector<Eigen::Vector3d>());
    offsets_to_test->reserve(pve_nsamples);
    for (size_t x_idx = 0; x_idx != pve_os_ratio; ++x_idx) {
      const default_type x =
          -0.5 + ((static_cast<default_type>(x_idx) + 0.5) / static_cast<default_type>(pve_os_ratio));
      for (size_t y_idx = 0; y_idx != pve_os_ratio; ++y_idx) {
        const default_type y =
            -0.5 + ((static_cast<default_type>(y_idx) + 0.5) / static_cast<default_type>(pve_os_ratio));
        for (size_t z_idx = 0; z_idx != pve_os_ratio; ++z_idx) {
          const default_type z =
              -0.5 + ((static_cast<default_type>(z_idx) + 0.5) / static_cast<default_type>(pve_os_ratio));
          offsets_to_test->push_back(Vertex(x, y, z));
        }
      }
    }
  }

  bool operator()(const std::pair<Vox, std::vector<size_t>> &in, std::pair<Vox, float> &out) const {
    const Vox &voxel(in.first);

    // Count the number of these points that lie inside the mesh
    size_t inside_mesh_count = 0;
    for (std::vector<Vertex>::const_iterator i_p = offsets_to_test->begin(); i_p != offsets_to_test->end(); ++i_p) {
      Vertex p(*i_p);
      p += Eigen::Vector3d(voxel[0], voxel[1], voxel[2]);

      default_type best_min_edge_distance_on_plane = -std::numeric_limits<default_type>::infinity();
      bool best_result_inside = false;
      default_type best_min_distance_from_interior_projection = std::numeric_limits<default_type>::infinity();

      // Only test against those polygons that are near this voxel
      for (std::vector<size_t>::const_iterator polygon_index = in.second.begin(); polygon_index != in.second.end();
           ++polygon_index) {
        const Eigen::Vector3d &n(polygon_normals[*polygon_index]);

        const size_t polygon_num_vertices = (*polygon_index < mesh.num_triangles()) ? 3 : 4;
        VertexList v;

        bool is_inside = false;
        default_type min_edge_distance_on_plane = std::numeric_limits<default_type>::infinity();
        default_type distance_from_plane = 0.0;

        // FIXME
        // If point does not lie within projection of polygon, compute the
        //   distance of the point projected onto the plane to the nearest edge of that polygon;
        //   use this distance to decide which polygon classifies the point
        // If point does lie within projection of polygon (potentially more than one), then the
        //   polygon to which the distance from the plane is minimal classifies the point

        if (polygon_num_vertices == 3) {

          mesh.load_triangle_vertices(v, *polygon_index);

          // First: is it aligned with the normal?
          const Vertex poly_centre((v[0] + v[1] + v[2]) * (1.0 / 3.0));
          const Vertex diff(p - poly_centre);
          distance_from_plane = diff.dot(n);
          is_inside = (distance_from_plane <= 0.0);

          // Second: how well does it project onto this polygon?
          const Vertex p_on_plane(p - (n * (diff.dot(n))));

          std::array<default_type, 3> edge_distances;
          Vertex zero = (v[1] - v[2]).cross(n);
          zero.normalize();
          Vertex one = (v[2] - v[0]).cross(n);
          one.normalize();
          Vertex two = (v[0] - v[1]).cross(n);
          two.normalize();
          edge_distances[0] = (p_on_plane - v[2]).dot(zero);
          edge_distances[1] = (p_on_plane - v[0]).dot(one);
          edge_distances[2] = (p_on_plane - v[1]).dot(two);
          min_edge_distance_on_plane = std::min({edge_distances[0], edge_distances[1], edge_distances[2]});

        } else {

          mesh.load_quad_vertices(v, *polygon_index);

          // This may be slightly ill-posed with a quad; no guarantee of fixed normal
          // Proceed regardless

          // First: is it aligned with the normal?
          const Vertex poly_centre((v[0] + v[1] + v[2] + v[3]) * 0.25);
          const Vertex diff(p - poly_centre);
          distance_from_plane = diff.dot(n);
          is_inside = (distance_from_plane <= 0.0);

          // Second: how well does it project onto this polygon?
          const Vertex p_on_plane(p - (n * (diff.dot(n))));

          for (int edge = 0; edge != 4; ++edge) {
            // Want an appropriate vector emanating from this edge from which to test the 'on-plane' distance
            //   (bearing in mind that there may not be a uniform normal)
            // For this, I'm going to take a weighted average based on the relative distance between the
            //   two points at either end of this edge
            // Edge is between points p1 and p2; edge 0 is between points 0 and 1
            const Vertex &p0((edge - 1) >= 0 ? v[edge - 1] : v[3]);
            const Vertex &p1(v[edge]);
            const Vertex &p2((edge + 1) < 4 ? v[edge + 1] : v[0]);
            const Vertex &p3((edge + 2) < 4 ? v[edge + 2] : v[edge - 2]);

            const default_type d1 = (p1 - p_on_plane).norm();
            const default_type d2 = (p2 - p_on_plane).norm();
            // Give more weight to the normal at the point that's closer
            Vertex edge_normal = (d2 * (p0 - p1) + d1 * (p3 - p2));
            edge_normal.normalize();

            // Now, how far away is the point within the plane from this edge?
            const default_type this_edge_distance = (p_on_plane - p1).dot(edge_normal);
            min_edge_distance_on_plane = std::min(min_edge_distance_on_plane, this_edge_distance);
          }
        }

        if (min_edge_distance_on_plane > 0.0) {
          if (std::fabs(distance_from_plane) < std::fabs(best_min_distance_from_interior_projection)) {
            best_min_distance_from_interior_projection = distance_from_plane;
            best_result_inside = is_inside;
          }
        } else if (!std::isfinite(best_min_distance_from_interior_projection)) {
          if (min_edge_distance_on_plane > best_min_edge_distance_on_plane) {
            best_min_edge_distance_on_plane = min_edge_distance_on_plane;
            best_result_inside = is_inside;
          }
        }
      }

      if (best_result_inside)
        ++inside_mesh_count;
    }

    out = std::make_pair(voxel, static_cast<default_type>(inside_mesh_count) / static_cast<default_type>(pve_nsamples));
    return true;
  }

private:
  const Mesh &mesh;
  const std::vector<Eigen::Vector3d> &polygon_normals;

  std::shared_ptr<std::vector<Eigen::Vector3d>> offsets_to_test;
};

// Toblerone per-voxel partial volume estimator:
//   adaptively subdivide the voxel into approximately isotropic sub-voxels; sub-voxels that do not
//   straddle the surface are assigned by classifying their centre; sub-voxels that do straddle the
//   surface are resampled at a finer scale and each sub-sample classified by a reduced ray test.
class TobleronePipe {
public:
  TobleronePipe(const Mesh &mesh,
                const std::vector<Eigen::Vector3d> &polygon_normals,
                const Vox2Poly &voxel2poly,
                const Image<uint8_t> &voxel_class,
                const std::array<default_type, 3> &spacing,
                const default_type subvoxel_mm)
      : mesh(mesh),
        polygon_normals(polygon_normals),
        voxel2poly(voxel2poly),
        voxel_class(voxel_class),
        spacing(spacing),
        subvoxel_mm(subvoxel_mm) {}

  bool operator()(const std::pair<Vox, std::vector<size_t>> &in, std::pair<Vox, float> &out) const {
    const Vox &voxel(in.first);
    const std::vector<size_t> &polys(in.second);

    // Per-axis subdivision so that sub-voxels are approximately isotropic with edge ~subvoxel_mm
    std::array<int, 3> subdiv;
    for (size_t a = 0; a != 3; ++a)
      subdiv[a] = std::max(1, static_cast<int>(std::ceil(spacing[a] / subvoxel_mm)));
    const Vertex subhalf(0.5 / subdiv[0], 0.5 / subdiv[1], 0.5 / subdiv[2]);
    const default_type subvoxel_volume = 8.0 * subhalf[0] * subhalf[1] * subhalf[2];

    // The voxel occupies unit volume in voxel space, so the accumulated interior volume is the
    //   partial volume fraction directly
    default_type inside_volume = 0.0;
    for (int sx = 0; sx != subdiv[0]; ++sx) {
      const default_type cx = voxel[0] - 0.5 + (sx + 0.5) / subdiv[0];
      for (int sy = 0; sy != subdiv[1]; ++sy) {
        const default_type cy = voxel[1] - 0.5 + (sy + 0.5) / subdiv[1];
        for (int sz = 0; sz != subdiv[2]; ++sz) {
          const default_type cz = voxel[2] - 0.5 + (sz + 0.5) / subdiv[2];
          const Vertex centre(cx, cy, cz);
          if (patch_overlaps_box(centre, subhalf, polys, mesh)) {
            // Where the surface cleanly cuts the sub-voxel, estimate the interior partial volume
            //   as the exact convex polytope formed by clipping the sub-voxel against that plane;
            //   otherwise (folded / multiple patches) resample the sub-voxel at a finer scale
            const std::optional<std::pair<Vertex, Vertex>> plane = favourable_plane(centre, subhalf, polys);
            if (plane)
              inside_volume += box_plane_interior_volume(centre, subhalf, plane->first, plane->second);
            else
              inside_volume += subvoxel_volume * intersected_fraction(centre, subhalf, voxel, polys);
          } else if (classify_point(centre, voxel, voxel2poly, voxel_class, mesh) == VC_INSIDE) {
            inside_volume += subvoxel_volume;
          }
        }
      }
    }
    out = std::make_pair(voxel, static_cast<float>(inside_volume));
    return true;
  }

private:
  const Mesh &mesh;
  const std::vector<Eigen::Vector3d> &polygon_normals;
  const Vox2Poly &voxel2poly;
  mutable Image<uint8_t> voxel_class;
  std::array<default_type, 3> spacing;
  default_type subvoxel_mm;

  // If the local patch cuts the sub-voxel as a single clean plane (no folds or opposing patches),
  //   return that plane as (point on surface, outward normal); otherwise std::nullopt
  std::optional<std::pair<Vertex, Vertex>>
  favourable_plane(const Vertex &centre, const Vertex &half, const std::vector<size_t> &polys) const {
    Vertex n_sum(0.0, 0.0, 0.0);
    std::vector<size_t> local;
    for (const size_t poly_index : polys) {
      if (polygon_box_overlap(centre, half, poly_index, mesh)) {
        local.push_back(poly_index);
        n_sum += polygon_normals[poly_index];
      }
    }
    if (local.empty() || n_sum.norm() < 1e-6)
      return std::nullopt;
    const Vertex n(n_sum.normalized());
    for (const size_t poly_index : local) {
      if (polygon_normals[poly_index].dot(n) < 0.5)
        return std::nullopt;
    }
    // Representative on-surface point: mean of the patch intersections with the sub-voxel edges
    std::array<Vertex, 8> c;
    for (int i = 0; i != 8; ++i)
      c[i] = Vertex(centre[0] + ((i & 1) ? half[0] : -half[0]),
                    centre[1] + ((i & 2) ? half[1] : -half[1]),
                    centre[2] + ((i & 4) ? half[2] : -half[2]));
    static const std::array<std::array<int, 2>, 12> box_edges = {
        {{0, 1}, {2, 3}, {4, 5}, {6, 7}, {0, 2}, {1, 3}, {4, 6}, {5, 7}, {0, 4}, {1, 5}, {2, 6}, {3, 7}}};
    std::vector<Vertex> pts;
    for (const auto &e : box_edges)
      segment_patch_intersections(c[e[0]], c[e[1]], local, mesh, pts);
    if (pts.size() < 3)
      return std::nullopt;
    Vertex q(0.0, 0.0, 0.0);
    for (const Vertex &p : pts)
      q += p;
    q /= static_cast<default_type>(pts.size());
    return std::make_pair(q, n);
  }

  // Fraction of a surface-straddling sub-voxel lying inside the surface, by resampling its centre
  //   region at a fixed finer scale and classifying each sub-sample with the reduced ray test
  default_type intersected_fraction(const Vertex &centre,
                                    const Vertex &half,
                                    const Vox &voxel,
                                    const std::vector<size_t> &polys) const {
    constexpr int f = toblerone_resample_factor;
    size_t inside = 0;
    for (int ax = 0; ax != f; ++ax) {
      const default_type px = centre[0] - half[0] + (ax + 0.5) * (2.0 * half[0] / f);
      for (int ay = 0; ay != f; ++ay) {
        const default_type py = centre[1] - half[1] + (ay + 0.5) * (2.0 * half[1] / f);
        for (int az = 0; az != f; ++az) {
          const default_type pz = centre[2] - half[2] + (az + 0.5) * (2.0 * half[2] / f);
          if (classify_point(Vertex(px, py, pz), voxel, voxel2poly, voxel_class, mesh) == VC_INSIDE)
            ++inside;
        }
      }
    }
    return static_cast<default_type>(inside) / static_cast<default_type>(Math::pow3(f));
  }
};

// Writes computed partial volume fractions back into the output image; shared by all variants
class Sink {
public:
  Sink(Image<float> &image, const size_t voxel_count)
      : image(image), progress("Calculating partial volume fractions of edge voxels", voxel_count) {}

  bool operator()(const std::pair<Vox, float> &in) {
    assign_pos_of(in.first).to(image);
    assert(!is_out_of_bounds(image));
    image.value() = in.second;
    ++progress;
    return true;
  }

private:
  Image<float> image;
  ProgressBar progress;
};

//
// ===================== Toblerone whole-voxel classification (scanline parity) =====================
//

namespace {

// Classify every voxel not straddling the surface as interior or exterior, by ray-casting parity
//   along each image row that contains surface-intersected voxels. Writes 1.0 (interior) / 0.0
//   (exterior) into the output image for these voxels, and records the ternary classification
//   (interior / exterior / on-mesh) into voxel_class for use by the partial-volume pipe.
// Surface-intersected voxels are left at 0.0 in the output image and marked VC_ON_MESH; their
//   partial volume fractions are computed subsequently by the pipe.
void scanline_parity_fill(Image<float> &image,
                          Image<uint8_t> &voxel_class,
                          const Vox2Poly &voxel2poly,
                          const Mesh &mesh) {
  const int size0 = static_cast<int>(image.size(0));

  for (auto l = Loop(voxel_class)(voxel_class, image); l; ++l) {
    voxel_class.value() = VC_OUTSIDE;
    image.value() = 0.0f;
  }
  for (const auto &vp : voxel2poly) {
    assign_pos_of(vp.first).to(voxel_class);
    voxel_class.value() = VC_ON_MESH;
  }

  // The map is ordered z, then y, then x, so all voxels of one (z, y) row are contiguous
  Vox2Poly::const_iterator it = voxel2poly.begin();
  while (it != voxel2poly.end()) {
    const int y = it->first[1];
    const int z = it->first[2];

    std::set<size_t> row_polys;
    Vox2Poly::const_iterator row_end = it;
    while (row_end != voxel2poly.end() && row_end->first[1] == y && row_end->first[2] == z) {
      row_polys.insert(row_end->second.begin(), row_end->second.end());
      ++row_end;
    }

    // Intersections of the (jittered) row centre-line with the surface
    std::vector<default_type> crossings;
    const Vertex ray_origin(-1.0, y + ray_jitter_y, z + ray_jitter_z);
    VertexList v;
    for (const size_t poly_index : row_polys) {
      if (poly_index < mesh.num_triangles()) {
        mesh.load_triangle_vertices(v, poly_index);
        const std::optional<default_type> t = ray_x_triangle(ray_origin, v[0], v[1], v[2]);
        if (t)
          crossings.push_back(ray_origin[0] + *t);
      } else {
        mesh.load_quad_vertices(v, poly_index - mesh.num_triangles());
        const std::optional<default_type> t0 = ray_x_triangle(ray_origin, v[0], v[1], v[2]);
        if (t0)
          crossings.push_back(ray_origin[0] + *t0);
        const std::optional<default_type> t1 = ray_x_triangle(ray_origin, v[0], v[2], v[3]);
        if (t1)
          crossings.push_back(ray_origin[0] + *t1);
      }
    }
    std::sort(crossings.begin(), crossings.end());

    size_t ci = 0;
    bool inside = false;
    Vox vox(0, y, z);
    for (int x = 0; x != size0; ++x) {
      while (ci < crossings.size() && crossings[ci] < static_cast<default_type>(x)) {
        inside = !inside;
        ++ci;
      }
      vox[0] = x;
      assign_pos_of(vox).to(voxel_class);
      if (voxel_class.value() == VC_ON_MESH)
        continue;
      voxel_class.value() = inside ? VC_INSIDE : VC_OUTSIDE;
      assign_pos_of(vox).to(image);
      image.value() = inside ? 1.0f : 0.0f;
    }
    it = row_end;
  }
}

} // namespace

//
// ================================= Public entry point =================================
//

void mesh2image(const Mesh &mesh_realspace, Image<float> &image, const Mesh2ImageOptions &options) {

  if (image.ndim() < 3)
    throw Exception("Template voxel grid for mesh2image operation must be at least 3D");

  // For speed, want the vertex data to be in voxel positions
  Mesh mesh;
  std::vector<Eigen::Vector3d> polygon_normals;
  Vox2Poly voxel2poly;

  Header H(image);
  H.ndim() = 3;

  // ===== Stage 1 (shared): identify the local surface patch intersecting each voxel =====
  // This is equivalent to Toblerone's first step (Moller triangle-box overlap), and is also the
  //   set of surface-intersected voxels for which the brute-force method computes fractions.
  {
    ProgressBar progress("Determining intersection of surface with voxel grid", 3);

    Filter::VertexTransform transform(image);
    transform.set_real2voxel();
    transform(mesh_realspace, mesh);
    ++progress;

    // These are needed for the interior-filling section of either algorithm
    if (!mesh.have_normals())
      mesh.calculate_normals();

    // Compute normals for polygons
    polygon_normals.reserve(mesh.num_polygons());
    for (TriangleList::const_iterator p = mesh.get_triangles().begin(); p != mesh.get_triangles().end(); ++p)
      polygon_normals.push_back(normal(mesh, *p));
    for (QuadList::const_iterator p = mesh.get_quads().begin(); p != mesh.get_quads().end(); ++p)
      polygon_normals.push_back(normal(mesh, *p));
    ++progress;

    // Map each polygon to the underlying voxels
    for (size_t poly_index = 0; poly_index != mesh.num_polygons(); ++poly_index) {

      const size_t num_vertices = (poly_index < mesh.num_triangles()) ? 3 : 4;

      // Figure out the voxel extent of this polygon in three dimensions
      Vox lower_bound(H.size(0) - 1, H.size(1) - 1, H.size(2) - 1), upper_bound(0, 0, 0);
      VertexList this_poly_verts;
      if (num_vertices == 3)
        mesh.load_triangle_vertices(this_poly_verts, poly_index);
      else
        mesh.load_quad_vertices(this_poly_verts, poly_index - mesh.num_triangles());
      for (VertexList::const_iterator v = this_poly_verts.begin(); v != this_poly_verts.end(); ++v) {
        for (size_t axis = 0; axis != 3; ++axis) {
          const int this_axis_voxel = std::round((*v)[axis]);
          lower_bound[axis] = std::min(lower_bound[axis], this_axis_voxel);
          upper_bound[axis] = std::max(upper_bound[axis], this_axis_voxel);
        }
      }

      // Constrain to lie within the dimensions of the image
      for (size_t axis = 0; axis != 3; ++axis) {
        lower_bound[axis] = std::max(0, lower_bound[axis]);
        upper_bound[axis] = std::min(static_cast<int>(H.size(axis) - 1), upper_bound[axis]);
      }

      // For all voxels within this rectangular region, assign this polygon to the map
      // Use the Separating Axis Theorem to be more stringent as to which voxels this
      //   polygon will be processed in
      auto overlap = [&](const Vox &vox, const size_t poly_index) -> bool {
        VertexList vertices;
        if (num_vertices == 3)
          mesh.load_triangle_vertices(vertices, poly_index);
        else
          mesh.load_quad_vertices(vertices, poly_index - mesh.num_triangles());

        // Test whether or not the two objects can be separated via projection onto an axis
        auto separating_axis = [&](const Eigen::Vector3d &axis) -> bool {
          default_type voxel_low = std::numeric_limits<default_type>::infinity();
          default_type voxel_high = -std::numeric_limits<default_type>::infinity();
          default_type poly_low = std::numeric_limits<default_type>::infinity();
          default_type poly_high = -std::numeric_limits<default_type>::infinity();

          static const std::array<Eigen::Vector3d, 8> voxel_offsets = {Eigen::Vector3d(-0.5, -0.5, -0.5),
                                                                       Eigen::Vector3d(-0.5, -0.5, 0.5),
                                                                       Eigen::Vector3d(-0.5, 0.5, -0.5),
                                                                       Eigen::Vector3d(-0.5, 0.5, 0.5),
                                                                       Eigen::Vector3d(0.5, -0.5, -0.5),
                                                                       Eigen::Vector3d(0.5, -0.5, 0.5),
                                                                       Eigen::Vector3d(0.5, 0.5, -0.5),
                                                                       Eigen::Vector3d(0.5, 0.5, 0.5)};

          for (size_t i = 0; i != 8; ++i) {
            const Eigen::Vector3d v(vox.matrix().cast<default_type>() + voxel_offsets[i]);
            const default_type projection = axis.dot(v);
            voxel_low = std::min(voxel_low, projection);
            voxel_high = std::max(voxel_high, projection);
          }

          for (const auto &v : vertices) {
            const default_type projection = axis.dot(v);
            poly_low = std::min(poly_low, projection);
            poly_high = std::max(poly_high, projection);
          }

          // Is this a separating axis?
          return (poly_low > voxel_high || voxel_low > poly_high);
        };

        // The following axes need to be tested as potential separating axes:
        //   x, y, z
        //   All cross-products between voxel and polygon edges
        //   Polygon normal
        for (size_t i = 0; i != 3; ++i) {
          Eigen::Vector3d axis(0.0, 0.0, 0.0);
          axis[i] = 1.0;
          if (separating_axis(axis))
            return false;
          for (size_t j = 0; j != num_vertices - 1; ++j) {
            if (separating_axis(axis.cross(vertices[j + 1] - vertices[j])))
              return false;
          }
          if (separating_axis(axis.cross(vertices[num_vertices - 1] - vertices[0])))
            return false;
        }
        if (separating_axis(polygon_normals[poly_index]))
          return false;

        // No axis has been found that separates the two objects
        // Therefore, the two objects overlap
        return true;
      };

      Vox voxel;
      for (voxel[2] = lower_bound[2]; voxel[2] <= upper_bound[2]; ++voxel[2]) {
        for (voxel[1] = lower_bound[1]; voxel[1] <= upper_bound[1]; ++voxel[1]) {
          for (voxel[0] = lower_bound[0]; voxel[0] <= upper_bound[0]; ++voxel[0]) {
            // Rather than adding this polygon to the list of polygons to test for
            //   every single voxel within this 3D bounding box, only test it within
            //   those voxels that the polygon actually intersects
            if (overlap(voxel, poly_index)) {
              std::vector<size_t> this_voxel_polys;
              // Has this voxel already been intersected by at least one polygon?
              // If it has, we need to concatenate this polygon to the list
              //   (which involves deleting the existing entry then re-writing the concatenated list)
              Vox2Poly::const_iterator existing = voxel2poly.find(voxel);
              if (existing != voxel2poly.end()) {
                this_voxel_polys = existing->second;
                voxel2poly.erase(existing);
              }
              this_voxel_polys.push_back(poly_index);
              voxel2poly.insert(std::make_pair(voxel, this_voxel_polys));
            }
          }
        }
      }
    }
    ++progress;
  }

  // ===== Stage 2: classify whole voxels, then compute edge-voxel partial volume fractions =====
  switch (options.method) {

  case Mesh2ImageMethod::BRUTE_FORCE: {

    // For initial segmentation of mesh - identify voxels on the mesh, inside & outside
    enum vox_mesh_t { UNDEFINED, ON_MESH, PRELIM_OUTSIDE, PRELIM_INSIDE, FILL_TEMP, OUTSIDE, INSIDE };

    static const std::array<Vox, 6> adj_voxels = {
        Vox(-1, 0, 0), Vox(+1, 0, 0), Vox(0, -1, 0), Vox(0, +1, 0), Vox(0, 0, -1), Vox(0, 0, +1)};

    ProgressBar progress("Performing voxel-based segmentation of surface", 5);

    Header Hseg(H);
    Hseg.datatype() = DataType::UInt8;
    auto init_seg = Image<uint8_t>::scratch(Hseg);
    for (auto l = Loop(init_seg)(init_seg); l; ++l)
      init_seg.value() = vox_mesh_t::UNDEFINED;
    for (const auto &vp : voxel2poly) {
      assign_pos_of(vp.first).to(init_seg);
      init_seg.value() = vox_mesh_t::ON_MESH;
    }
    ++progress;

    // For *any* voxel not on the mesh but neighbouring a voxel in which a vertex lies,
    //   track a floating-point value corresponding to its distance from the plane defined
    //   by the normal at the vertex.
    // Each voxel not directly on the mesh should then be assigned as prelim_inside or prelim_outside
    //   depending on whether the summed value is positive or negative
    Hseg.datatype() = DataType::Float32;
    Hseg.datatype().set_byte_order_native();
    auto sum_distances = Image<float>::scratch(Hseg, "Sum of distances from polygon planes");
    Vox adj_voxel;
    for (size_t i = 0; i != mesh.num_vertices(); ++i) {
      const Vox centre_voxel(mesh.vert(i));
      for (adj_voxel[2] = centre_voxel[2] - 1; adj_voxel[2] <= centre_voxel[2] + 1; ++adj_voxel[2]) {
        for (adj_voxel[1] = centre_voxel[1] - 1; adj_voxel[1] <= centre_voxel[1] + 1; ++adj_voxel[1]) {
          for (adj_voxel[0] = centre_voxel[0] - 1; adj_voxel[0] <= centre_voxel[0] + 1; ++adj_voxel[0]) {
            if (!is_out_of_bounds(H, adj_voxel, 0, 3) && (adj_voxel - centre_voxel).any()) {
              const Eigen::Vector3d offset(adj_voxel.cast<default_type>().matrix() - mesh.vert(i));
              const default_type dp_normal = offset.dot(mesh.norm(i));
              const default_type offset_on_plane = (offset - (mesh.norm(i) * dp_normal)).norm();
              assign_pos_of(adj_voxel).to(sum_distances);
              // If offset_on_plane is close to zero, this vertex should contribute strongly toward
              //   the sum of distances from the surface within this voxel
              sum_distances.value() += (1.0 / (1.0 + offset_on_plane)) * dp_normal;
            }
          }
        }
      }
    }
    ++progress;
    for (auto l = Loop(init_seg)(init_seg, sum_distances); l; ++l) {
      if (static_cast<float>(sum_distances.value()) != 0.0f && init_seg.value() != vox_mesh_t::ON_MESH)
        init_seg.value() = sum_distances.value() < 0.0 ? vox_mesh_t::PRELIM_INSIDE : vox_mesh_t::PRELIM_OUTSIDE;
    }

    // Can't guarantee that mesh might have a single isolated polygon pointing the wrong way
    // Therefore, need to:
    //   - Select voxels both inside and outside the mesh to expand
    //   - When expanding each region, count the number of pre-assigned voxels both inside and outside
    //   - For the final region selection, assign values to voxels based on a majority vote
    Image<uint8_t> seed(init_seg);
    std::vector<Vox> to_fill;
    std::stack<Vox> to_expand;
    for (auto l = Loop(seed)(seed); l; ++l) {
      if (seed.value() == vox_mesh_t::PRELIM_INSIDE || seed.value() == vox_mesh_t::PRELIM_OUTSIDE) {
        size_t prelim_inside_count = 0, prelim_outside_count = 0;
        float sum_sum_distances = 0.0f;
        if (seed.value() == vox_mesh_t::PRELIM_INSIDE)
          prelim_inside_count = 1;
        else
          prelim_outside_count = 1;
        to_expand.push(Vox(seed.index(0), seed.index(1), seed.index(2)));
        to_fill.assign(1, to_expand.top());
        do {
          const Vox voxel(to_expand.top());
          to_expand.pop();
          for (size_t adj_vox_idx = 0; adj_vox_idx != 6; ++adj_vox_idx) {
            const Vox adj_voxel(voxel + adj_voxels[adj_vox_idx]);
            assign_pos_of(adj_voxel).to(init_seg);
            if (!is_out_of_bounds(init_seg)) {
              const uint8_t adj_value = init_seg.value();
              if (adj_value == vox_mesh_t::UNDEFINED || adj_value == vox_mesh_t::PRELIM_INSIDE ||
                  adj_value == vox_mesh_t::PRELIM_OUTSIDE) {
                if (adj_value == vox_mesh_t::PRELIM_INSIDE)
                  ++prelim_inside_count;
                else if (adj_value == vox_mesh_t::PRELIM_OUTSIDE)
                  ++prelim_outside_count;
                assign_pos_of(init_seg).to(sum_distances);
                sum_sum_distances += sum_distances.value();
                to_expand.push(adj_voxel);
                to_fill.push_back(adj_voxel);
                init_seg.value() = vox_mesh_t::FILL_TEMP;
              }
            }
          }
        } while (!to_expand.empty());
        vox_mesh_t fill_value = vox_mesh_t::UNDEFINED;
        if (prelim_inside_count == prelim_outside_count && sum_sum_distances) {
          fill_value = sum_sum_distances < 0.0f ? vox_mesh_t::INSIDE : vox_mesh_t::OUTSIDE;
        } else if (prelim_inside_count > 10 * prelim_outside_count) {
          fill_value = vox_mesh_t::INSIDE;
        } else if (prelim_outside_count > 10 * prelim_inside_count) {
          fill_value = vox_mesh_t::OUTSIDE;
        } else {
          // Residual ambiguity about whether the connected region is inside or outside the surface
          // What other tests can we perform to make this decision?
          // If all eight corners of the FoV are included in to_fill, we can be
          //   reasonably confident that this connected region lies outside the structure
          size_t corner_count = 0;
          for (const auto &voxel : to_fill) {
            if ((voxel[0] == 0 || voxel[0] == H.size(0) - 1) && (voxel[1] == 0 || voxel[1] == H.size(1) - 1) &&
                (voxel[2] == 0 || voxel[2] == H.size(2) - 1))
              ++corner_count;
          }
          if (corner_count == 8) {
            fill_value = vox_mesh_t::OUTSIDE;
          } else if (!corner_count) {
            fill_value = vox_mesh_t::INSIDE;
          } else if (sum_sum_distances != 0.0F) {
            fill_value = sum_sum_distances < 0.0F ? vox_mesh_t::INSIDE : vox_mesh_t::OUTSIDE;
          } else {
            Exception e("Internal error: fundamental ambiguity in voxel-based segmentation of surface");
            e.push_back("Fill region size: " + str(to_fill.size()));
            e.push_back("Preliminary classifications: " + str(prelim_inside_count) + " inside, " +
                        str(prelim_outside_count) + " outside");
            e.push_back("FoV corners: " + str(corner_count));
            throw e;
          }
        }
        for (const auto &voxel : to_fill) {
          assign_pos_of(voxel).to(init_seg);
          init_seg.value() = fill_value;
        }
        to_fill.clear();
      }
    }
    ++progress;

    // Any voxel not yet processed must lie outside the structure(s)
    for (auto l = Loop(init_seg)(init_seg); l; ++l) {
      if (init_seg.value() == vox_mesh_t::UNDEFINED)
        init_seg.value() = vox_mesh_t::OUTSIDE;
    }
    ++progress;

    // Write initial ternary segmentation
    for (auto l = Loop(init_seg)(init_seg, image); l; ++l) {
      switch (init_seg.value()) {
      case vox_mesh_t(UNDEFINED):
        throw Exception("Code error: poor filling of initial mesh estimate");
        break;
      case vox_mesh_t(ON_MESH):
        image.value() = 0.5;
        break;
      case vox_mesh_t(OUTSIDE):
        image.value() = 0.0;
        break;
      case vox_mesh_t(INSIDE):
        image.value() = 1.0;
        break;
      default:
        assert(0);
      }
    }

    // For each voxel intersected by the surface, calculate the partial volume fraction
    Source source(voxel2poly);
    BruteForcePipe pipe(mesh, polygon_normals);
    Sink sink(image, voxel2poly.size());
    Thread::run_queue(
        source, std::pair<Vox, std::vector<size_t>>(), Thread::multi(pipe), std::pair<Vox, float>(), sink);

  } break;

  case Mesh2ImageMethod::TOBLERONE: {

    Header Hclass(H);
    Hclass.datatype() = DataType::UInt8;
    auto voxel_class = Image<uint8_t>::scratch(Hclass, "voxel interior/exterior classification");
    scanline_parity_fill(image, voxel_class, voxel2poly, mesh);

    const std::array<default_type, 3> spacing = {H.spacing(0), H.spacing(1), H.spacing(2)};
    const default_type subvoxel_mm = options.subvoxel_mm.value_or(default_subvoxel_mm);

    Source source(voxel2poly);
    TobleronePipe pipe(mesh, polygon_normals, voxel2poly, voxel_class, spacing, subvoxel_mm);
    Sink sink(image, voxel2poly.size());
    Thread::run_queue(
        source, std::pair<Vox, std::vector<size_t>>(), Thread::multi(pipe), std::pair<Vox, float>(), sink);

  } break;
  }
}

} // namespace MR::Surface::Algo
