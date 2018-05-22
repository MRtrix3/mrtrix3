/*
 * Copyright (c) 2008-2018 the MRtrix3 contributors.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, you can obtain one at http://mozilla.org/MPL/2.0/
 *
 * MRtrix3 is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * For more details, see http://www.mrtrix.org/
 */


#include "surface/algo/mesh2image.h"

#include <map>

#include "header.h"
#include "progressbar.h"
#include "thread_queue.h"
#include "types.h"

#include "surface/types.h"
#include "surface/utils.h"
#include "surface/filter/vertex_transform.h"

namespace MR
{
  namespace Surface
  {
    namespace Algo
    {


      constexpr size_t pve_os_ratio = 10;
      constexpr size_t pve_nsamples = Math::pow3 (pve_os_ratio);


      void mesh2image (const Mesh& mesh_realspace, Image<float>& image)
      {

        // For initial segmentation of mesh - identify voxels on the mesh, inside & outside
        enum vox_mesh_t { UNDEFINED, ON_MESH, PRELIM_OUTSIDE, PRELIM_INSIDE, FILL_TEMP, OUTSIDE, INSIDE };

        ProgressBar progress ("converting mesh to partial volume image", 8);

        // For speed, want the vertex data to be in voxel positions
        Filter::VertexTransform transform (image);
        transform.set_real2voxel();
        Mesh mesh;
        transform (mesh_realspace, mesh);

        // These are needed now for interior filling section of algorithm
        if (!mesh.have_normals())
          mesh.calculate_normals();

        static const Vox adj_voxels[6] = { { -1,  0,  0 },
                                           { +1,  0,  0 },
                                           {  0, -1,  0 },
                                           {  0, +1,  0 },
                                           {  0,  0, -1 },
                                           {  0,  0, +1 } };

        // Compute normals for polygons
        vector<Eigen::Vector3> polygon_normals;
        polygon_normals.reserve (mesh.num_polygons());
        for (TriangleList::const_iterator p = mesh.get_triangles().begin(); p != mesh.get_triangles().end(); ++p)
          polygon_normals.push_back (normal (mesh, *p));
        for (QuadList::const_iterator p = mesh.get_quads().begin(); p != mesh.get_quads().end(); ++p)
          polygon_normals.push_back (normal (mesh, *p));
        ++progress;

        // Create some memory to work with:
        // Stores a flag for each voxel as encoded in enum vox_mesh_t
        Header H (image);
        auto init_seg = Image<uint8_t>::scratch (H);
        for (auto l = Loop(init_seg) (init_seg); l; ++l)
          init_seg.value() = vox_mesh_t::UNDEFINED;

        // For every voxel, stores those polygons that may intersect the voxel
        using Vox2Poly = std::map< Vox, vector<size_t> >;
        Vox2Poly voxel2poly;

        // Map each polygon to the underlying voxels
        for (size_t poly_index = 0; poly_index != mesh.num_polygons(); ++poly_index) {

          const size_t num_vertices = (poly_index < mesh.num_triangles()) ? 3 : 4;

          // Figure out the voxel extent of this polygon in three dimensions
          Vox lower_bound (H.size(0)-1, H.size(1)-1, H.size(2)-1), upper_bound (0, 0, 0);
          VertexList this_poly_verts;
          if (num_vertices == 3)
            mesh.load_triangle_vertices (this_poly_verts, poly_index);
          else
            mesh.load_quad_vertices (this_poly_verts, poly_index - mesh.num_triangles());
          for (VertexList::const_iterator v = this_poly_verts.begin(); v != this_poly_verts.end(); ++v) {
            for (size_t axis = 0; axis != 3; ++axis) {
              const int this_axis_voxel = std::round((*v)[axis]);
              lower_bound[axis] = std::min (lower_bound[axis], this_axis_voxel);
              upper_bound[axis] = std::max (upper_bound[axis], this_axis_voxel);
            }
          }

          // Constrain to lie within the dimensions of the image
          for (size_t axis = 0; axis != 3; ++axis) {
            lower_bound[axis] = std::max(0,                   lower_bound[axis]);
            upper_bound[axis] = std::min(int(H.size(axis)-1), upper_bound[axis]);
          }

          // For all voxels within this rectangular region, assign this polygon to the map
          // Use the Separating Axis Theorem to be more stringent as to which voxels this
          //   polygon will be processed in
          auto overlap = [&] (const Vox& vox, const size_t poly_index) -> bool {

            VertexList vertices;
            if (num_vertices == 3)
              mesh.load_triangle_vertices (vertices, poly_index);
            else
              mesh.load_quad_vertices (vertices, poly_index - mesh.num_triangles());

            // Test whether or not the two objects can be separated via projection onto an axis
            auto separating_axis = [&] (const Eigen::Vector3& axis) -> bool {
              default_type voxel_low  =  std::numeric_limits<default_type>::infinity();
              default_type voxel_high = -std::numeric_limits<default_type>::infinity();
              default_type poly_low   =  std::numeric_limits<default_type>::infinity();
              default_type poly_high  = -std::numeric_limits<default_type>::infinity();

              static const Eigen::Vector3 voxel_offsets[8] = { { -0.5, -0.5, -0.5 },
                                                               { -0.5, -0.5,  0.5 },
                                                               { -0.5,  0.5, -0.5 },
                                                               { -0.5,  0.5,  0.5 },
                                                               {  0.5, -0.5, -0.5 },
                                                               {  0.5, -0.5,  0.5 },
                                                               {  0.5,  0.5, -0.5 },
                                                               {  0.5,  0.5,  0.5 } };

              for (size_t i = 0; i != 8; ++i) {
                const Eigen::Vector3 v (vox.matrix().cast<default_type>() + voxel_offsets[i]);
                const default_type projection = axis.dot (v);
                voxel_low  = std::min (voxel_low,  projection);
                voxel_high = std::max (voxel_high, projection);
              }

              for (const auto& v : vertices) {
                const default_type projection = axis.dot (v);
                poly_low  = std::min (poly_low,  projection);
                poly_high = std::max (poly_high, projection);
              }

              // Is this a separating axis?
              return (poly_low > voxel_high || voxel_low > poly_high);
            };

            // The following axes need to be tested as potential separating axes:
            //   x, y, z
            //   All cross-products between voxel and polygon edges
            //   Polygon normal
            for (size_t i = 0; i != 3; ++i) {
              Eigen::Vector3 axis (0.0, 0.0, 0.0);
              axis[i] = 1.0;
              if (separating_axis (axis))
                return false;
              for (size_t j = 0; j != num_vertices; ++j) {
                if (separating_axis (axis.cross (vertices[j+1] - vertices[j])))
                  return false;
              }
              if (separating_axis (axis.cross (vertices[num_vertices-1] - vertices[0])))
                return false;
            }
            if (separating_axis (polygon_normals[poly_index]))
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
                if (overlap (voxel, poly_index)) {
                  vector<size_t> this_voxel_polys;
                  // Has this voxel already been intersected by at least one polygon?
                  // If it has, we need to concatenate this polygon to the list
                  //   (which involves deleting the existing entry then re-writing the concatenated list);
                  // If it has not, we're adding a new entry to the list of voxels to be tested,
                  //   with only one entry in the list for that voxel
                  Vox2Poly::const_iterator existing = voxel2poly.find (voxel);
                  if (existing != voxel2poly.end()) {
                    this_voxel_polys = existing->second;
                    voxel2poly.erase (existing);
                  } else {
                    // Only call this once each voxel, regardless of the number of intersecting polygons
                    assign_pos_of (voxel).to (init_seg);
                    init_seg.value() = vox_mesh_t::ON_MESH;
                  }
                  this_voxel_polys.push_back (poly_index);
                  voxel2poly.insert (std::make_pair (voxel, this_voxel_polys));
          } } } }

        }
        ++progress;


        // For *any* voxel not on the mesh but neighbouring a voxel in which a vertex lies,
        //   track a floating-point value corresponding to its distance from the plane defined
        //   by the normal at the vertex.
        // Each voxel not directly on the mesh should then be assigned as prelim_inside or prelim_outside
        //   depending on whether the summed value is positive or negative
        auto sum_distances = Image<float>::scratch (H, "Sum of distances from polygon planes");
        Vox adj_voxel;
        for (size_t i = 0; i != mesh.num_vertices(); ++i) {
          const Vox centre_voxel (mesh.vert(i));
          for (adj_voxel[2] = centre_voxel[2]-1; adj_voxel[2] <= centre_voxel[2]+1; ++adj_voxel[2]) {
            for (adj_voxel[1] = centre_voxel[1]-1; adj_voxel[1] <= centre_voxel[1]+1; ++adj_voxel[1]) {
              for (adj_voxel[0] = centre_voxel[0]-1; adj_voxel[0] <= centre_voxel[0]+1; ++adj_voxel[0]) {
                if (!is_out_of_bounds (H, adj_voxel) && (adj_voxel - centre_voxel).any()) {
                  const Eigen::Vector3 offset (adj_voxel.cast<default_type>().matrix() - mesh.vert(i));
                  const default_type dp_normal = offset.dot (mesh.norm(i));
                  const default_type offset_on_plane = (offset - (mesh.norm(i) * dp_normal)).norm();
                  assign_pos_of (adj_voxel).to (sum_distances);
                  // If offset_on_plane is close to zero, this vertex should contribute strongly toward
                  //   the sum of distances from the surface within this voxel
                  sum_distances.value() += (1.0 / (1.0 + offset_on_plane)) * dp_normal;
                }
              }
            }
          }
        }
        ++progress;
        for (auto l = Loop(init_seg) (init_seg, sum_distances); l; ++l) {
          if (static_cast<float> (sum_distances.value()) != 0.0f && init_seg.value() != vox_mesh_t::ON_MESH)
            init_seg.value() = sum_distances.value() < 0.0 ? vox_mesh_t::PRELIM_INSIDE : vox_mesh_t::PRELIM_OUTSIDE;
        }
        ++progress;


        // Can't guarantee that mesh might have a single isolated polygon pointing the wrong way
        // Therefore, need to:
        //   - Select voxels both inside and outside the mesh to expand
        //   - When expanding each region, count the number of pre-assigned voxels both inside and outside
        //   - For the final region selection, assign values to voxels based on a majority vote
        Image<uint8_t> seed (init_seg);
        vector<Vox> to_fill;
        std::stack<Vox> to_expand;
        for (auto l = Loop(seed) (seed); l; ++l) {
          if (seed.value() == vox_mesh_t::PRELIM_INSIDE || seed.value() == vox_mesh_t::PRELIM_OUTSIDE) {
            size_t prelim_inside_count = 0, prelim_outside_count = 0;
            if (seed.value() == vox_mesh_t::PRELIM_INSIDE)
              prelim_inside_count = 1;
            else
              prelim_outside_count = 1;
            to_expand.push (Vox (seed.index(0), seed.index(1), seed.index(2)));
            to_fill.assign (1, to_expand.top());
            do {
              const Vox voxel (to_expand.top());
              to_expand.pop();
              for (size_t adj_vox_idx = 0; adj_vox_idx != 6; ++adj_vox_idx) {
                const Vox adj_voxel (voxel + adj_voxels[adj_vox_idx]);
                assign_pos_of (adj_voxel).to (init_seg);
                if (!is_out_of_bounds (init_seg)) {
                  const uint8_t adj_value = init_seg.value();
                  if (adj_value == vox_mesh_t::UNDEFINED || adj_value == vox_mesh_t::PRELIM_INSIDE || adj_value == vox_mesh_t::PRELIM_OUTSIDE) {
                    if (adj_value == vox_mesh_t::PRELIM_INSIDE)
                      ++prelim_inside_count;
                    else if (adj_value == vox_mesh_t::PRELIM_OUTSIDE)
                      ++prelim_outside_count;
                    to_expand.push (adj_voxel);
                    to_fill.push_back (adj_voxel);
                    init_seg.value() = vox_mesh_t::FILL_TEMP;
                  }
                }
              }
            } while (to_expand.size());
            if (prelim_inside_count == prelim_outside_count)
              throw Exception ("Mapping mesh to image failed: Unable to label connected voxel region as inside or outside mesh");
            const vox_mesh_t fill_value = (prelim_inside_count > prelim_outside_count ? vox_mesh_t::INSIDE : vox_mesh_t::OUTSIDE);
            for (auto voxel : to_fill) {
              assign_pos_of (voxel).to (init_seg);
              init_seg.value() = fill_value;
            }
            to_fill.clear();
          }
        }
        ++progress;

        // Any voxel not yet processed must lie outside the structure(s)
        for (auto l = Loop(init_seg) (init_seg); l; ++l) {
          if (init_seg.value() == vox_mesh_t::UNDEFINED)
            init_seg.value() = vox_mesh_t::OUTSIDE;
        }
        ++progress;

        // Write initial ternary segmentation
        for (auto l = Loop (init_seg) (init_seg, image); l; ++l) {
          switch (init_seg.value()) {
            case vox_mesh_t (UNDEFINED): throw Exception ("Code error: poor filling of initial mesh estimate"); break;
            case vox_mesh_t (ON_MESH):   image.value() = 0.5; break;
            case vox_mesh_t (OUTSIDE):   image.value() = 0.0; break;
            case vox_mesh_t (INSIDE):    image.value() = 1.0; break;
            default: assert (0);
          }
        }
        ++progress;

        // Construct class functors necessary to calculate, for each voxel intersected by the
        //   surface, the partial volume fraction
        class Source
        { MEMALIGN(Source)
          public:
            Source (const Vox2Poly& data) :
                data (data),
                i (data.begin()) { }

            bool operator() (std::pair<Vox, vector<size_t>>& out)
            {
              if (i == data.end())
                return false;
              out = std::make_pair (i->first, i->second);
              ++i;
              return true;
            }

          private:
            const Vox2Poly& data;
            Vox2Poly::const_iterator i;
        };

        class Pipe
        { NOMEMALIGN
          public:
            Pipe (const Mesh& mesh, const vector<Eigen::Vector3>& polygon_normals) :
                mesh (mesh),
                polygon_normals (polygon_normals)

            {
              // Generate a set of points within this voxel that need to be tested individually
              offsets_to_test.reset(new vector<Eigen::Vector3>());
              offsets_to_test->reserve (pve_nsamples);
              for (size_t x_idx = 0; x_idx != pve_os_ratio; ++x_idx) {
                const default_type x = -0.5 + ((default_type(x_idx) + 0.5) / default_type(pve_os_ratio));
                for (size_t y_idx = 0; y_idx != pve_os_ratio; ++y_idx) {
                  const default_type y = -0.5 + ((default_type(y_idx) + 0.5) / default_type(pve_os_ratio));
                  for (size_t z_idx = 0; z_idx != pve_os_ratio; ++z_idx) {
                    const default_type z = -0.5 + ((default_type(z_idx) + 0.5) / default_type(pve_os_ratio));
                    offsets_to_test->push_back (Vertex (x, y, z));
                  }
                }
              }
            }

            bool operator() (const std::pair<Vox, vector<size_t>>& in, std::pair<Vox, float>& out) const
            {
              const Vox& voxel (in.first);

              // Count the number of these points that lie inside the mesh
              size_t inside_mesh_count = 0;
              for (vector<Vertex>::const_iterator i_p = offsets_to_test->begin(); i_p != offsets_to_test->end(); ++i_p) {
                Vertex p (*i_p);
                p += Eigen::Vector3 (voxel[0], voxel[1], voxel[2]);

                default_type best_min_edge_distance_on_plane = -std::numeric_limits<default_type>::infinity();
                //default_type best_interior_distance_from_plane = std::numeric_limits<default_type>::infinity();
                bool best_result_inside = false;

                // Only test against those polygons that are near this voxel
                for (vector<size_t>::const_iterator polygon_index = in.second.begin(); polygon_index != in.second.end(); ++polygon_index) {
                  const Eigen::Vector3& n (polygon_normals[*polygon_index]);

                  const size_t polygon_num_vertices = (*polygon_index < mesh.num_triangles()) ? 3 : 4;
                  VertexList v;

                  bool is_inside = false;
                  default_type min_edge_distance_on_plane = std::numeric_limits<default_type>::infinity();

                  // FIXME
                  // If point does not lie within projection of polygon, compute the
                  //   distance of the point projected onto the plane to the nearest edge of that polygon;
                  //   use this distance to decide which polygon classifies the point
                  // If point does lie within projection of polygon (potentially more than one), then the
                  //   polygon to which the distance from the plane is minimal classifies the point

                  if (polygon_num_vertices == 3) {

                    mesh.load_triangle_vertices (v, *polygon_index);

                    // First: is it aligned with the normal?
                    const Vertex poly_centre ((v[0] + v[1] + v[2]) * (1.0/3.0));
                    const Vertex diff (p - poly_centre);
                    is_inside = (diff.dot (n) <= 0.0);

                    // Second: how well does it project onto this polygon?
                    const Vertex p_on_plane (p - (n * (diff.dot (n))));

                    std::array<default_type, 3> edge_distances;
                    Vertex zero = (v[2]-v[0]).cross (n); zero.normalize();
                    Vertex one  = (v[1]-v[2]).cross (n); one .normalize();
                    Vertex two  = (v[0]-v[1]).cross (n); two .normalize();
                    edge_distances[0] = (p_on_plane-v[0]).dot (zero);
                    edge_distances[1] = (p_on_plane-v[2]).dot (one);
                    edge_distances[2] = (p_on_plane-v[1]).dot (two);
                    min_edge_distance_on_plane = std::min ( { edge_distances[0], edge_distances[1], edge_distances[2] } );

                  } else {

                    mesh.load_quad_vertices (v, *polygon_index);

                    // This may be slightly ill-posed with a quad; no guarantee of fixed normal
                    // Proceed regardless

                    // First: is it aligned with the normal?
                    const Vertex poly_centre ((v[0] + v[1] + v[2] + v[3]) * 0.25);
                    const Vertex diff (p - poly_centre);
                    is_inside = (diff.dot (n) <= 0.0);

                    // Second: how well does it project onto this polygon?
                    const Vertex p_on_plane (p - (n * (diff.dot (n))));

                    for (int edge = 0; edge != 4; ++edge) {
                      // Want an appropriate vector emanating from this edge from which to test the 'on-plane' distance
                      //   (bearing in mind that there may not be a uniform normal)
                      // For this, I'm going to take a weighted average based on the relative distance between the
                      //   two points at either end of this edge
                      // Edge is between points p1 and p2; edge 0 is between points 0 and 1
                      const Vertex& p0 ((edge-1) >= 0 ? v[edge-1] : v[3]);
                      const Vertex& p1 (v[edge]);
                      const Vertex& p2 ((edge+1) < 4 ? v[edge+1] : v[0]);
                      const Vertex& p3 ((edge+2) < 4 ? v[edge+2] : v[edge-2]);

                      const default_type d1 = (p1 - p_on_plane).norm();
                      const default_type d2 = (p2 - p_on_plane).norm();
                      // Give more weight to the normal at the point that's closer
                      Vertex edge_normal = (d2*(p0-p1) + d1*(p3-p2));
                      edge_normal.normalize();

                      // Now, how far away is the point within the plane from this edge?
                      const default_type this_edge_distance = (p_on_plane - p1).dot (edge_normal);
                      min_edge_distance_on_plane = std::min (min_edge_distance_on_plane, this_edge_distance);

                    }

                  }

                  if (min_edge_distance_on_plane > best_min_edge_distance_on_plane) {
                    best_min_edge_distance_on_plane = min_edge_distance_on_plane;
                    best_result_inside = is_inside;
                  }

                }

                if (best_result_inside)
                  ++inside_mesh_count;

              }

              out = std::make_pair (voxel, (default_type)inside_mesh_count / (default_type)pve_nsamples);
              return true;
            }

          private:
            const Mesh& mesh;
            const vector<Eigen::Vector3>& polygon_normals;

            std::shared_ptr<vector<Eigen::Vector3>> offsets_to_test;

        };

        class Sink
        { MEMALIGN(Sink)
          public:
            Sink (Image<float>& image) :
                image (image) { }

            bool operator() (const std::pair<Vox, float>& in)
            {
              assign_pos_of (in.first).to (image);
              assert (!is_out_of_bounds (image));
              image.value() = in.second;
              return true;
            }

          private:
            Image<float> image;

        };

        Source source (voxel2poly);
        Pipe pipe (mesh, polygon_normals);
        Sink sink (image);

        Thread::run_queue (source,
                           std::pair<Vox, vector<size_t>>(),
                           Thread::multi (pipe),
                           std::pair<Vox, float>(),
                           sink);
      }




    }
  }
}


