/* Copyright (c) 2008-2017 the MRtrix3 contributors.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, you can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * MRtrix is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * For more details, see http://www.mrtrix.org/.
 */


#include "surface/algo/mesh2image.h"

#include <map>
#include <vector>

#include "header.h"
#include "progressbar.h"

#include "surface/types.h"
#include "surface/utils.h"
#include "surface/filter/vertex_transform.h"

namespace MR
{
  namespace Surface
  {
    namespace Algo
    {



      void mesh2image (const Mesh& mesh_realspace, Image<float>& image)
      {

        // For initial segmentation of mesh - identify voxels on the mesh, inside & outside
        enum vox_mesh_t { UNDEFINED, ON_MESH, OUTSIDE, INSIDE };

        ProgressBar progress ("converting mesh to PVE image", 6);

        // For speed, want the vertex data to be in voxel positions
        Filter::VertexTransform transform (image);
        transform.set_real2voxel();
        Mesh mesh;
        transform (mesh_realspace, mesh);

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
          Vox voxel;
          for (voxel[2] = lower_bound[2]; voxel[2] <= upper_bound[2]; ++voxel[2]) {
            for (voxel[1] = lower_bound[1]; voxel[1] <= upper_bound[1]; ++voxel[1]) {
              for (voxel[0] = lower_bound[0]; voxel[0] <= upper_bound[0]; ++voxel[0]) {
                vector<size_t> this_voxel_polys;
                //#if __clang__
                //              Vox2Poly::const_iterator existing = voxel2poly.find (voxel);
                //#else
                //              Vox2Poly::iterator existing = voxel2poly.find (voxel);
                //#endif
                Vox2Poly::const_iterator existing = voxel2poly.find (voxel);
                if (existing != voxel2poly.end()) {
                  this_voxel_polys = existing->second;
                  voxel2poly.erase (existing);
                } else {
                  // Only call this once each voxel, regardless of the number of intersecting polygons
                  assign_pos_of (voxel).to (init_seg);
                  init_seg.value() = ON_MESH;
                }
                this_voxel_polys.push_back (poly_index);
                voxel2poly.insert (std::make_pair (voxel, this_voxel_polys));
          } } }

        }
        ++progress;


        // Find all voxels that are not partial-volumed with the mesh, and are not inside the mesh
        // Use a corner of the image FoV to commence filling of the volume, and then check that all
        //   eight corners have been flagged as outside the volume
        const Vox corner_voxels[8] = {
            Vox (             0,              0,              0),
            Vox (             0,              0, H.size (2) - 1),
            Vox (             0, H.size (1) - 1,              0),
            Vox (             0, H.size (1) - 1, H.size (2) - 1),
            Vox (H.size (0) - 1,              0,              0),
            Vox (H.size (0) - 1,              0, H.size (2) - 1),
            Vox (H.size (0) - 1, H.size (1) - 1,              0),
            Vox (H.size (0) - 1, H.size (1) - 1, H.size (2) - 1)};

        // TODO This is slow; is there a faster implementation?
        // This is essentially a connected-component analysis...
        vector<Vox> to_expand;
        to_expand.push_back (corner_voxels[0]);
        assign_pos_of (corner_voxels[0]).to (init_seg);
        init_seg.value() = vox_mesh_t::OUTSIDE;
        do {
          const Vox centre_voxel (to_expand.back());
          to_expand.pop_back();
          for (size_t adj_vox_idx = 0; adj_vox_idx != 6; ++adj_vox_idx) {
            const Vox this_voxel (centre_voxel + adj_voxels[adj_vox_idx]);
            assign_pos_of (this_voxel).to (init_seg);
            if (!is_out_of_bounds (init_seg) && init_seg.value() == vox_mesh_t (UNDEFINED)) {
              init_seg.value() = vox_mesh_t (OUTSIDE);
              to_expand.push_back (this_voxel);
            }
          }
        } while (!to_expand.empty());
        ++progress;

        for (size_t cnr_idx = 0; cnr_idx != 8; ++cnr_idx) {
          assign_pos_of (corner_voxels[cnr_idx]).to (init_seg);
          if (init_seg.value() == vox_mesh_t (UNDEFINED))
            throw Exception ("Mesh is not bound within image field of view");
        }


        // Find those voxels that remain unassigned, and set them to INSIDE
        for (auto l = Loop (init_seg) (init_seg); l; ++l) {
          if (init_seg.value() == vox_mesh_t (UNDEFINED))
            init_seg.value() = vox_mesh_t (INSIDE);
        }
        ++progress;

        // Write initial ternary segmentation
        for (auto l = Loop (init_seg) (init_seg, image); l; ++l) {
          switch (init_seg.value()) {
            case vox_mesh_t (UNDEFINED): throw Exception ("Code error: poor filling of initial mesh estimate"); break;
            case vox_mesh_t (ON_MESH):   image.value() = 0.5; break;
            case vox_mesh_t (OUTSIDE):   image.value() = 0.0; break;
            case vox_mesh_t (INSIDE):    image.value() = 1.0; break;
          }
        }
        ++progress;

        // Get better partial volume estimates for all necessary voxels
        // TODO This could be multi-threaded, but hard to justify the dev time
        static const size_t pve_os_ratio = 10;

        for (Vox2Poly::const_iterator i = voxel2poly.begin(); i != voxel2poly.end(); ++i) {

          const Vox& voxel (i->first);

          // Generate a set of points within this voxel that need to be tested individually
          vector<Vertex> to_test;
          to_test.reserve (Math::pow3 (pve_os_ratio));
          for (size_t x_idx = 0; x_idx != pve_os_ratio; ++x_idx) {
            const default_type x = voxel[0] - 0.5 + ((default_type(x_idx) + 0.5) / default_type(pve_os_ratio));
            for (size_t y_idx = 0; y_idx != pve_os_ratio; ++y_idx) {
              const default_type y = voxel[1] - 0.5 + ((default_type(y_idx) + 0.5) / default_type(pve_os_ratio));
              for (size_t z_idx = 0; z_idx != pve_os_ratio; ++z_idx) {
                const default_type z = voxel[2] - 0.5 + ((default_type(z_idx) + 0.5) / default_type(pve_os_ratio));
                to_test.push_back (Vertex (x, y, z));
              }
            }
          }

          // Count the number of these points that lie inside the mesh
          int inside_mesh_count = 0;
          for (vector<Vertex>::const_iterator i_p = to_test.begin(); i_p != to_test.end(); ++i_p) {
            const Vertex& p (*i_p);

            default_type best_min_edge_distance = -std::numeric_limits<default_type>::infinity();
            bool best_result_inside = false;

            // Only test against those polygons that are near this voxel
            for (vector<size_t>::const_iterator polygon_index = i->second.begin(); polygon_index != i->second.end(); ++polygon_index) {
              const Eigen::Vector3& n (polygon_normals[*polygon_index]);

              const size_t polygon_num_vertices = (*polygon_index < mesh.num_triangles()) ? 3 : 4;
              VertexList v;

              bool is_inside = false;
              default_type min_edge_distance = std::numeric_limits<default_type>::infinity();

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
                min_edge_distance = std::min (edge_distances[0], std::min (edge_distances[1], edge_distances[2]));

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
                  min_edge_distance = std::min (min_edge_distance, this_edge_distance);

                }

              }

              if (min_edge_distance > best_min_edge_distance) {
                best_min_edge_distance = min_edge_distance;
                best_result_inside = is_inside;
              }

            }

            if (best_result_inside)
              ++inside_mesh_count;

          }

          assign_pos_of (voxel).to (image);
          image.value() = (default_type)inside_mesh_count / (default_type)Math::pow3 (pve_os_ratio);

        }

      }




    }
  }
}


