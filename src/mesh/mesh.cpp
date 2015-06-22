/*
   Copyright 2011 Brain Research Institute, Melbourne, Australia

   Written by Robert E. Smith, 2011.

   This file is part of MRtrix.

   MRtrix is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   MRtrix is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with MRtrix.  If not, see <http://www.gnu.org/licenses/>.

*/


#include "mesh/mesh.h"

#include "get_set.h"


#include <ctime>
#include <vector>
#include <set>


namespace MR
{
  namespace Mesh
  {

    namespace {
      // Little class needed for face data reading for OBJ files
      struct FaceData {
          uint32_t vertex, texture, normal;
      };
    }



    template <>
    bool Polygon<3>::shares_edge (const Polygon<3>& that) const
    {
      uint32_t shared_vertices = 0;
      if (indices[0] == that[0] || indices[0] == that[1] || indices[0] == that[2]) ++shared_vertices;
      if (indices[1] == that[0] || indices[1] == that[1] || indices[1] == that[2]) ++shared_vertices;
      if (indices[2] == that[0] || indices[2] == that[1] || indices[2] == that[2]) ++shared_vertices;
      return (shared_vertices == 2);
    }




    Mesh::Mesh (const std::string& path)
    {
      if (path.substr (path.size() - 4) == ".vtk" || path.substr (path.size() - 4) == ".VTK")
        load_vtk (path);
      else if (path.substr (path.size() - 4) == ".stl" || path.substr (path.size() - 4) == ".STL")
        load_stl (path);
      else if (path.substr (path.size() - 4) == ".obj" || path.substr (path.size() - 4) == ".OBJ")
        load_obj (path);
      else
        throw Exception ("Input mesh file not in supported format");
      name = path;
    }



    void Mesh::transform_first_to_realspace (const Image::Info& info)
    {
      Image::Transform transform (info);
      for (VertexList::iterator v = vertices.begin(); v != vertices.end(); ++v) {
        (*v)[0] = ((info.dim(0)-1) * info.vox(0)) - (*v)[0];
        *v = transform.image2scanner (*v);
      }
      if (normals.size()) {
        for (VertexList::iterator n = normals.begin(); n != normals.end(); ++n) {
          (*n)[0] = -(*n)[0];
          *n = transform.image2scanner_dir (*n);
        }
      }
    }

    void Mesh::transform_voxel_to_realspace (const Image::Info& info)
    {
      Image::Transform transform (info);
      for (VertexList::iterator v = vertices.begin(); v != vertices.end(); ++v)
        *v = transform.voxel2scanner (*v);
      if (normals.size()) {
        for (VertexList::iterator n = normals.begin(); n != normals.end(); ++n)
          *n = transform.voxel2scanner_dir (*n);
      }
    }

    void Mesh::transform_realspace_to_voxel (const Image::Info& info)
    {
      Image::Transform transform (info);
      for (VertexList::iterator v = vertices.begin(); v != vertices.end(); ++v)
        *v = transform.scanner2voxel (*v);
      if (normals.size()) {
        for (VertexList::iterator n = normals.begin(); n != normals.end(); ++n)
          *n = transform.scanner2voxel_dir (*n);
      }
    }



    void Mesh::save (const std::string& path, const bool binary) const
    {
      if (path.substr (path.size() - 4) == ".vtk")
        save_vtk (path, binary);
      else if (path.substr (path.size() - 4) == ".stl")
        save_stl (path, binary);
      else if (path.substr (path.size() - 4) == ".obj")
        save_obj (path);
      else
        throw Exception ("Output mesh file format not supported");
    }




    void Mesh::output_pve_image (const Image::Header& H, const std::string& path)
    {

      // For initial segmentation of mesh - identify voxels on the mesh, inside & outside
      enum vox_mesh_t { UNDEFINED, ON_MESH, OUTSIDE, INSIDE };

      ProgressBar progress ("converting mesh to PVE image... ", 7);

      // For speed, want the vertex data to be in voxel positions
      // Therefore modify the vertex data in place, but save the original data and set it
      //   back to the way it was on function completion
      const VertexList vertices_realspace (vertices);
      transform_realspace_to_voxel (H);

      static const Point<int> adj_voxels[6] = { Point<int> (-1,  0,  0),
                                                Point<int> (+1,  0,  0),
                                                Point<int> ( 0, -1,  0),
                                                Point<int> ( 0, +1,  0),
                                                Point<int> ( 0,  0, -1),
                                                Point<int> ( 0,  0, +1)};

      // Compute normals for polygons
      std::vector< Point<float> > polygon_normals;
      normals.reserve (triangles.size() + quads.size());
      for (TriangleList::const_iterator p = triangles.begin(); p != triangles.end(); ++p)
        polygon_normals.push_back (calc_normal (*p));
      for (QuadList::const_iterator p = quads.begin(); p != quads.end(); ++p)
        polygon_normals.push_back (calc_normal (*p));

      // Create some memory to work with:
      // Stores a flag for each voxel as encoded in enum vox_mesh_t
      Image::BufferScratch<uint8_t> init_seg_data (H);
      auto init_seg  = init_seg_data.voxel();

      // For every voxel, stores those polygons that may intersect the voxel
      typedef std::map< Point<int>, std::vector<size_t> > Vox2Poly;
      Vox2Poly voxel2poly;

      // Map each polygon to the underlying voxels
      for (size_t poly_index = 0; poly_index != num_polygons(); ++poly_index) {

        const size_t num_vertices = (poly_index < triangles.size()) ? 3 : 4;

        // Figure out the voxel extent of this polygon in three dimensions
        Point<int> lower_bound (H.dim(0)-1, H.dim(1)-1, H.dim(2)-1), upper_bound (0, 0, 0);
        VertexList this_poly_verts;
        if (num_vertices == 3)
          load_triangle_vertices (this_poly_verts, poly_index);
        else
          load_quad_vertices (this_poly_verts, poly_index - triangles.size());
        for (VertexList::const_iterator v = this_poly_verts.begin(); v != this_poly_verts.end(); ++v) {
          for (size_t axis = 0; axis != 3; ++axis) {
            const int this_axis_voxel = std::round((*v)[axis]);
            lower_bound[axis] = std::min (lower_bound[axis], this_axis_voxel);
            upper_bound[axis] = std::max (upper_bound[axis], this_axis_voxel);
          }
        }

        // Constrain to lie within the dimensions of the image
        for (size_t axis = 0; axis != 3; ++axis) {
          lower_bound[axis] = std::max(0,             lower_bound[axis]);
          upper_bound[axis] = std::min(H.dim(axis)-1, upper_bound[axis]);
        }

        // For all voxels within this rectangular region, assign this polygon to the map
        Point<int> voxel;
        for (voxel[2] = lower_bound[2]; voxel[2] <= upper_bound[2]; ++voxel[2]) {
          for (voxel[1] = lower_bound[1]; voxel[1] <= upper_bound[1]; ++voxel[1]) {
            for (voxel[0] = lower_bound[0]; voxel[0] <= upper_bound[0]; ++voxel[0]) {
              std::vector<size_t> this_voxel_polys;
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
                Image::Nav::set_value_at_pos (init_seg, voxel, ON_MESH);
              }
              this_voxel_polys.push_back (poly_index);
              voxel2poly.insert (std::make_pair (voxel, this_voxel_polys));
        } } }

      }
      ++progress;


      // Find all voxels that are not partial-volumed with the mesh, and are not inside the mesh
      // Use a corner of the image FoV to commence filling of the volume, and then check that all
      //   eight corners have been flagged as outside the volume
      const Point<int> corner_voxels[8] = {
          Point<int> (            0,             0,             0),
          Point<int> (            0,             0, H.dim (2) - 1),
          Point<int> (            0, H.dim (1) - 1,             0),
          Point<int> (            0, H.dim (1) - 1, H.dim (2) - 1),
          Point<int> (H.dim (0) - 1,             0,             0),
          Point<int> (H.dim (0) - 1,             0, H.dim (2) - 1),
          Point<int> (H.dim (0) - 1, H.dim (1) - 1,             0),
          Point<int> (H.dim (0) - 1, H.dim (1) - 1, H.dim (2) - 1)};

      // TODO This is slow; is there a faster implementation?
      // This is essentially a connected-component analysis...
      std::vector< Point<int> > to_expand;
      to_expand.push_back (corner_voxels[0]);
      Image::Nav::set_value_at_pos (init_seg, corner_voxels[0], vox_mesh_t(OUTSIDE));
      do {
        const Point<int> centre_voxel (to_expand.back());
        to_expand.pop_back();
        for (size_t adj_vox_idx = 0; adj_vox_idx != 6; ++adj_vox_idx) {
          const Point<int> this_voxel (centre_voxel + adj_voxels[adj_vox_idx]);
          Image::Nav::set_pos (init_seg, this_voxel);
          if (Image::Nav::within_bounds (init_seg) && init_seg.value() == vox_mesh_t (UNDEFINED)) {
            init_seg.value() = vox_mesh_t (OUTSIDE);
            to_expand.push_back (this_voxel);
          }
        }
      } while (!to_expand.empty());
      ++progress;

      for (size_t cnr_idx = 0; cnr_idx != 8; ++cnr_idx) {
        if (Image::Nav::get_value_at_pos (init_seg, corner_voxels[cnr_idx]) == vox_mesh_t (UNDEFINED))
          throw Exception ("Mesh is not bound within image field of view");
      }


      // Find those voxels that remain unassigned, and set them to INSIDE
      Image::Loop loop;
      for (auto l = loop (init_seg); l; ++l) {
        if (init_seg.value() == vox_mesh_t (UNDEFINED))
          init_seg.value() = vox_mesh_t (INSIDE);
      }
      ++progress;


      // Generate the initial estimated PVE image
      Image::BufferScratch<float> pve_est_data (H);
      auto pve_est = pve_est_data.voxel();

      for (auto l = loop (init_seg, pve_est); l; ++l) {
        switch (init_seg.value()) {
          case vox_mesh_t (UNDEFINED):   throw Exception ("Code error: poor filling of initial mesh estimate"); break;
          case vox_mesh_t (ON_MESH):     pve_est.value() = 0.5; break;
          case vox_mesh_t (OUTSIDE):     pve_est.value() = 0.0; break;
          case vox_mesh_t (INSIDE):      pve_est.value() = 1.0; break;
        }
      }
      ++progress;


      // Get better partial volume estimates for all necessary voxels
      // TODO This could be multi-threaded, but hard to justify the dev time
      static const size_t pve_os_ratio = 10;

      for (Vox2Poly::const_iterator i = voxel2poly.begin(); i != voxel2poly.end(); ++i) {

        const Point<int>& voxel (i->first);

        // Generate a set of points within this voxel that need to be tested individually
        std::vector< Point<float> > to_test;
        to_test.reserve (Math::pow3 (pve_os_ratio));
        for (size_t x_idx = 0; x_idx != pve_os_ratio; ++x_idx) {
          const float x = voxel[0] - 0.5 + ((float(x_idx) + 0.5) / float(pve_os_ratio));
          for (size_t y_idx = 0; y_idx != pve_os_ratio; ++y_idx) {
            const float y = voxel[1] - 0.5 + ((float(y_idx) + 0.5) / float(pve_os_ratio));
            for (size_t z_idx = 0; z_idx != pve_os_ratio; ++z_idx) {
              const float z = voxel[2] - 0.5 + ((float(z_idx) + 0.5) / float(pve_os_ratio));
              to_test.push_back (Point<float> (x, y, z));
            }
          }
        }

        // Count the number of these points that lie inside the mesh
        int inside_mesh_count = 0;
        for (std::vector< Point<float> >::const_iterator i_p = to_test.begin(); i_p != to_test.end(); ++i_p) {
          const Point<float>& p (*i_p);

          float best_min_edge_distance = -INFINITY;
          bool best_result_inside = false;

          // Only test against those polygons that are near this voxel
          for (std::vector<size_t>::const_iterator polygon_index = i->second.begin(); polygon_index != i->second.end(); ++polygon_index) {
            const Point<float>& n (polygon_normals[*polygon_index]);

            const size_t polygon_num_vertices = (*polygon_index < triangles.size()) ? 3 : 4;
            VertexList v;

            bool is_inside = false;
            float min_edge_distance = std::numeric_limits<float>::infinity();

            if (polygon_num_vertices == 3) {

              load_triangle_vertices (v, *polygon_index);

              // First: is it aligned with the normal?
              const Point<float> poly_centre ((v[0] + v[1] + v[2]) * (1.0 / 3.0));
              const Point<float> diff (p - poly_centre);
              is_inside = (diff.dot (n) <= 0.0);

              // Second: how well does it project onto this polygon?
              const Point<float> p_on_plane (p - (n * (diff.dot (n))));

              min_edge_distance = minvalue ((p_on_plane - v[0]).dot ((v[2]-v[0]).cross (n).normalise()),
                                            (p_on_plane - v[2]).dot ((v[1]-v[2]).cross (n).normalise()),
                                            (p_on_plane - v[1]).dot ((v[0]-v[1]).cross (n).normalise()));

            } else {

              load_quad_vertices (v, *polygon_index);

              // This may be slightly ill-posed with a quad; no guarantee of fixed normal
              // Proceed regardless

              // First: is it aligned with the normal?
              const Point<float> poly_centre ((v[0] + v[1] + v[2] + v[3]) * 0.25f);
              const Point<float> diff (p - poly_centre);
              is_inside = (diff.dot (n) <= 0.0);

              // Second: how well does it project onto this polygon?
              const Point<float> p_on_plane (p - (n * (diff.dot (n))));

              for (int edge = 0; edge != 4; ++edge) {
                // Want an appropriate vector emanating from this edge from which to test the 'on-plane' distance
                //   (bearing in mind that there may not be a uniform normal)
                // For this, I'm going to take a weighted average based on the relative distance between the
                //   two points at either end of this edge
                // Edge is between points p1 and p2; edge 0 is between points 0 and 1
                const Point<float>& p0 ((edge-1) >= 0 ? v[edge-1] : v[3]);
                const Point<float>& p1 (v[edge]);
                const Point<float>& p2 ((edge+1) < 4 ? v[edge+1] : v[0]);
                const Point<float>& p3 ((edge+2) < 4 ? v[edge+2] : v[edge-2]);

                const float d1 = (p1 - p_on_plane).norm();
                const float d2 = (p2 - p_on_plane).norm();
                // Give more weight to the normal at the point that's closer
                const Point<float> edge_normal = (d2*(p0-p1) + d1*(p3-p2)).normalise();

                // Now, how far away is the point within the plane from this edge?
                const float this_edge_distance = (p_on_plane - p1).dot (edge_normal);
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

        Image::Nav::set_value_at_pos (pve_est, voxel, (float)inside_mesh_count / (float)Math::pow3 (pve_os_ratio));

      }
      ++progress;

      // Write image to file
      pve_est.save (path);
      ++progress;

      // Restore the vertex data back to realspace
      vertices = vertices_realspace;

    }



    void Mesh::smooth (const float spatial_factor, const float influence_factor)
    {
      if (quads.size())
        throw Exception ("For now, mesh smoothing is only supported for triangular meshes");
      if (vertices.size() == 3 * vertices.size())
        throw Exception ("Cannot perform smoothing on this mesh: no triangulation information");

      ProgressBar progress ("Performing mesh smoothing... ", 2 * vertices.size() + 1);

      // Pre-compute polygon centroids and areas
      VertexList centroids;
      std::vector<float> areas;
      for (TriangleList::const_iterator p = triangles.begin(); p != triangles.end(); ++p) {
        centroids.push_back ((vertices[(*p)[0]] + vertices[(*p)[1]] + vertices[(*p)[2]]) * (1.0f/3.0f));
        areas.push_back (calc_area (*p));
      }
      for (QuadList::const_iterator p = quads.begin(); p != quads.end(); ++p) {
        centroids.push_back ((vertices[(*p)[0]] + vertices[(*p)[1]] + vertices[(*p)[2]] + vertices[(*p)[3]]) * 0.25f);
        areas.push_back (calc_area (*p));
      }

      // Perform pre-calculation of an appropriate mesh neighbourhood for each vertex
      // Use knowledge of the connections between vertices provided by the triangles/quads to
      //   perform an iterative search outward from each vertex, selecting a subset of
      //   polygons for each vertex
      // Extent of window should be approximately the value of spatial_factor, though only an
      //   approximate windowing is likely to be used (i.e. number of iterations)
      //
      // Initialisation is different to iterations: Need a single pass to find those
      //   polygons that actually use the vertex
      std::vector< std::set<uint32_t> > vert_polys (vertices.size(), std::set<uint32_t>());
      // For each vertex, don't just want to store the polygons within the neighbourhood;
      //   also want to store those that will be expanded from in the next iteration
      std::vector< std::vector<uint32_t> > vert_polys_to_expand (vertices.size(), std::vector<uint32_t>());

      for (uint32_t t = 0; t != triangles.size(); ++t) {
        for (uint32_t i = 0; i != 3; ++i) {
          vert_polys[(triangles[t])[i]].insert (t);
          vert_polys_to_expand[(triangles[t])[i]].push_back (t);
        }
      }

      // Now, we want to expand this selection outwards for each vertex
      // To do this, also want to produce a list for each polygon: containing those polygons
      //   that share a common edge (i.e. two vertices)
      std::vector< std::vector<uint32_t> > poly_neighbours (triangles.size(), std::vector<uint32_t>());
      for (uint32_t i = 0; i != triangles.size(); ++i) {
        for (uint32_t j = i+1; j != triangles.size(); ++j) {
          if (triangles[i].shares_edge (triangles[j])) {
            poly_neighbours[i].push_back (j);
            poly_neighbours[j].push_back (i);

          }
        }
      }

      // TODO Will want to develop a better heuristic for this
      for (size_t iter = 0; iter != 5; ++iter) {
        for (uint32_t v = 0; v != vertices.size(); ++v) {

          // Find polygons at the outer edge of this expanding front, and add them to the neighbourhood for this vertex
          std::vector<uint32_t> next_front;
          for (std::vector<uint32_t>::const_iterator front = vert_polys_to_expand[v].begin(); front != vert_polys_to_expand[v].end(); ++front) {
            for (std::vector<uint32_t>::const_iterator expansion = poly_neighbours[*front].begin(); expansion != poly_neighbours[*front].end(); ++expansion) {
              const std::set<uint32_t>::const_iterator existing = vert_polys[v].find (*expansion);
              if (existing == vert_polys[v].end()) {
                vert_polys[v].insert (*expansion);
                next_front.push_back (*expansion);
              }
            }
          }
          vert_polys_to_expand[v] = std::move (next_front);

        }
      }



      // Need to perform a first mollification pass, where the polygon normals are
      //   smoothed but the vertices are not perturbed
      // However, in order to calculate these new normals, we need to calculate new vertex positions!
      // Make a copy of the original vertices
      const VertexList orig_vertices (vertices);
      // Use half standard spatial factor for mollification
      // Denominator = 2(SF/2)^2
      const float spatial_mollification_power_multiplier = -2.0f / Math::pow2 (spatial_factor);
      // No need to normalise the Gaussian; have to explicitly normalise afterwards
      for (uint32_t v = 0; v != vertices.size(); ++v) {

        Vertex new_pos (0.0f, 0.0f, 0.0f);
        float sum_weights = 0.0f;

        // For now, just use every polygon as part of the estimate
        // Eventually, restrict this to some form of mesh neighbourhood
        //for (size_t i = 0; i != centroids.size(); ++i) {
        for (std::set<uint32_t>::const_iterator it = vert_polys[v].begin(); it != vert_polys[v].end(); ++it) {
          const uint32_t i = *it;
          float this_weight = areas[i];
          const float distance_sq = (centroids[i] - vertices[v]).norm2();
          this_weight *= std::exp (distance_sq * spatial_mollification_power_multiplier);
          const Vertex prediction = centroids[i];
          new_pos += this_weight * prediction;
          sum_weights += this_weight;
        }

        new_pos *= (1.0f / sum_weights);
        vertices[v] = new_pos;
        ++progress;

      }

      // Have new vertices; compute polygon normals based on these vertices
      VertexList tangents;
      for (TriangleList::const_iterator p = triangles.begin(); p != triangles.end(); ++p)
        tangents.push_back (calc_normal (*p));
      for (QuadList::const_iterator p = quads.begin(); p != quads.end(); ++p)
        tangents.push_back (calc_normal (*p));

      // Restore the original vertices
      vertices = orig_vertices;

      // Now perform the actual smoothing
      const float spatial_power_multiplier = -0.5f / Math::pow2 (spatial_factor);
      const float influence_power_multiplier = -0.5f / Math::pow2 (influence_factor);
      for (size_t v = 0; v != vertices.size(); ++v) {

        Vertex new_pos (0.0f, 0.0f, 0.0f);
        float sum_weights = 0.0f;

        //for (size_t i = 0; i != centroids.size(); ++i) {
        for (std::set<uint32_t>::const_iterator it = vert_polys[v].begin(); it != vert_polys[v].end(); ++it) {
          const uint32_t i = *it;
          float this_weight = areas[i];
          const float distance_sq = (centroids[i] - vertices[v]).norm2();
          this_weight *= std::exp (distance_sq * spatial_power_multiplier);
          const float prediction_distance = (centroids[i] - vertices[v]).dot (tangents[i]);
          const Vertex prediction = vertices[v] + (tangents[i] * prediction_distance);
          this_weight *= std::exp (Math::pow2 (prediction_distance) * influence_power_multiplier);
          new_pos += this_weight * prediction;
          sum_weights += this_weight;
        }

        new_pos *= (1.0f / sum_weights);
        vertices[v] = new_pos;
        ++progress;

      }

      // If the vertex normals were calculated previously, re-calculate them
      if (normals.size())
        calculate_normals();

    }




    void Mesh::calculate_normals()
    {
      normals.clear();
      normals.assign (vertices.size(), Vertex (0.0f, 0.0f, 0.0f));
      for (TriangleList::const_iterator p = triangles.begin(); p != triangles.end(); ++p) {
        const Vertex this_normal = calc_normal (*p);
        for (size_t index = 0; index != 3; ++index)
          normals[(*p)[index]] += this_normal;
      }
      for (QuadList::const_iterator p = quads.begin(); p != quads.end(); ++p) {
        const Vertex this_normal = calc_normal (*p);
        for (size_t index = 0; index != 4; ++index)
          normals[(*p)[index]] += this_normal;
      }
      for (VertexList::iterator n = normals.begin(); n != normals.end(); ++n)
        n->normalise();
    }





    void Mesh::load_vtk (const std::string& path)
    {

      std::ifstream in (path.c_str(), std::ios_base::in);
      if (!in)
        throw Exception ("Error opening input file!");

      std::string line;

      // First line: VTK version ID
      std::getline (in, line);
      // Strip the version numbers
      line[23] = line[25] = 'x';
      // Verify that the line is correct
      if (line != "# vtk DataFile Version x.x")
        throw Exception ("Incorrect first line of .vtk file");

      // Second line: identifier
      std::getline (in, line);

      // Third line: format of data
      std::getline (in, line);
      bool is_ascii = false;
      if (line == "ASCII")
        is_ascii = true;
      else if (line != "BINARY")
        throw Exception ("unknown data format in .vtk data");

      // Fourth line: Data set type
      std::getline (in, line);
      if (line.substr(0, 7) != "DATASET")
        throw Exception ("Error in definition of .vtk dataset");
      line = line.substr (8);
      if (line == "STRUCTURED_POINTS" || line == "STRUCTURED_GRID" || line == "UNSTRUCTURED_GRID" || line == "RECTILINEAR_GRID" || line == "FIELD")
        throw Exception ("Unsupported dataset type (" + line + ") in .vtk file");

      // From here, don't necessarily know which parts of the data will come first
      while (!in.eof()) {
        std::getline (in, line);

        if (line.size()) {
          if (line.substr (0, 6) == "POINTS") {

            line = line.substr (7);
            const size_t ws = line.find (' ');
            const int num_vertices = to<int> (line.substr (0, ws));
            line = line.substr (ws + 1);

            vertices.reserve (num_vertices);
            for (int i = 0; i != num_vertices; ++i) {

              Vertex v;
              if (is_ascii) {
                std::getline (in, line);
                sscanf (line.c_str(), "%f %f %f", &v[0], &v[1], &v[2]);
              } else {
                for (size_t i = 0; i != 3; ++i)
                  in.read (reinterpret_cast<char*>(&v[i]), sizeof (float));
              }
              vertices.push_back (v);

            }

          } else if (line.substr (0, 8) == "POLYGONS") {

            line = line.substr (9);
            const size_t ws = line.find (' ');
            const int num_polygons = to<int> (line.substr (0, ws));
            line = line.substr (ws + 1);
            const int num_elements = to<int> (line);

            int polygon_count = 0, element_count = 0;
            while (polygon_count < num_polygons && element_count < num_elements) {

              int vertex_count;
              if (is_ascii) {
                std::getline (in, line);
                sscanf (line.c_str(), "%u", &vertex_count);
              } else {
                in.read (reinterpret_cast<char*>(&vertex_count), sizeof (int));
              }

              if (vertex_count != 3 && vertex_count != 4)
                throw Exception ("Could not parse file \"" + path + "\";  only suppport 3- and 4-vertex polygons");

              std::vector<unsigned int> t (vertex_count, 0);

              if (is_ascii) {
                for (int index = 0; index != vertex_count; ++index) {
                  line = line.substr (line.find (' ') + 1); // Strip the previous value
                  sscanf (line.c_str(), "%u", &t[index]);
                }
              } else {
                for (int index = 0; index != vertex_count; ++index)
                  in.read (reinterpret_cast<char*>(&t[index]), sizeof (int));
              }
              if (vertex_count == 3)
                triangles.push_back (Polygon<3>(t));
              else
                quads.push_back (Polygon<4>(t));
              ++polygon_count;
              element_count += 1 + vertex_count;

            }
            if (polygon_count != num_polygons || element_count != num_elements)
              throw Exception ("Incorrectly read polygon data from .vtk file \"" + path + "\"");

          } else {
            throw Exception ("Unsupported data \"" + line + "\" in .vtk file \"" + path + "\"");
          }
        }
      }

      // TODO If reading a binary file, may want to test endianness of data
      // There's no explicit flag for this, but just calculating the standard
      //   deviations of all vertex positions may be adequate
      //   (likely to be huge if the endianness is wrong)
      // Alternatively, just test the polygon indices: if there's at least one that exceeds the
      //   number of vertices, it may be saved in big-endian format, so try flipping everything
      // Actually, should pop up at the first polygon read: number of points in polygon won't be 3 or 4

      verify_data();
    }



    void Mesh::load_stl (const std::string& path)
    {
      std::ifstream in (path.c_str(), std::ios_base::in);
      if (!in)
        throw Exception ("Error opening input file!");

      Point<float> normal;
      Vertex vertex;
      bool warn_right_hand_rule = false, warn_nonstandard_normals = false;

      char init[6];
      in.get (init, 6);

      if (strncmp (init, "solid ", 6)) {

        // File is stored as binary
        in.close();
        in.open (path.c_str(), std::ios_base::in | std::ios_base::binary);
        char header[80];
        in.read (header, 80);

        uint32_t count;
        in.read (reinterpret_cast<char*>(&count), sizeof(uint32_t));

        uint16_t attribute_byte_count;
        bool warn_attribute = false;

        while (in.read (reinterpret_cast<char*>(&normal[0]), 3 * sizeof(float))) {
          for (size_t index = 0; index != 3; ++index) {
            if (!in.read (reinterpret_cast<char*>(&vertex[0]), 3 * sizeof(float)))
              throw Exception ("Error in parsing STL file");
            vertices.push_back (vertex);
          }
          in.read (reinterpret_cast<char*>(&attribute_byte_count), sizeof(uint16_t));
          if (attribute_byte_count)
            warn_attribute = true;
          triangles.push_back ( std::vector<uint32_t> { uint32_t(vertices.size()-3), uint32_t(vertices.size()-2), uint32_t(vertices.size()-1) } );
          const Point<float> computed_normal = calc_normal (triangles.back());
          if (computed_normal.dot (normal) < 0.0)
            warn_right_hand_rule = true;
          if (std::abs (computed_normal.dot (normal)) < 0.99)
            warn_nonstandard_normals = true;
        }
        if (triangles.size() != count)
          WARN ("Number of triangles indicated in file " + Path::basename (path) + "(" + str(count) + ") does not match number actually read (" + str(triangles.size()) + ")");
        if (warn_attribute)
          WARN ("Some facets in file " + Path::basename (path) + " have extended attributes; ignoring");

      } else {

        // File is stored as ASCII
        std::string rest_of_header;
        std::getline (in, rest_of_header);

        std::string line;
        size_t vertex_index = 0;
        bool inside_facet = false, inside_loop = false;
        while (std::getline (in, line)) {
          // Strip leading whitespace
          line = line.substr (line.find_first_not_of (' '), line.npos);
          if (line.substr(12) == "facet normal") {
            if (inside_facet)
              throw Exception ("Error parsing STL file " + Path::basename (path) + ": nested facets");
            inside_facet = true;
            line = line.substr (12);
            sscanf (line.c_str(), "%f %f %f", &normal[0], &normal[1], &normal[2]);
          } else if (line.substr(10) == "outer loop") {
            if (inside_loop)
              throw Exception ("Error parsing STL file " + Path::basename (path) + ": nested loops");
            if (!inside_facet)
              throw Exception ("Error parsing STL file " + Path::basename (path) + ": loop outside facet");
            inside_loop = true;
          } else if (line.substr(6) == "vertex") {
            if (!inside_loop)
              throw Exception ("Error parsing STL file " + Path::basename (path) + ": vertex outside loop");
            if (!inside_facet)
              throw Exception ("Error parsing STL file " + Path::basename (path) + ": vertex outside facet");
            line = line.substr (6);
            sscanf (line.c_str(), "%f %f %f", &vertex[0], &vertex[1], &vertex[2]);
            vertices.push_back (vertex);
            ++vertex_index;
          } else if (line.substr() == "endloop") {
            if (!inside_loop)
              throw Exception ("Error parsing STL file " + Path::basename (path) + ": loop ending without start");
            if (!inside_facet)
              throw Exception ("Error parsing STL file " + Path::basename (path) + ": loop ending outside facet");
            inside_loop = false;
          } else if (line.substr() == "endfacet") {
            if (inside_loop)
              throw Exception ("Error parsing STL file " + Path::basename (path) + ": facet ending inside loop");
            if (!inside_facet)
              throw Exception ("Error parsing STL file " + Path::basename (path) + ": facet ending without start");
            inside_facet = false;
            if (vertex_index != 3)
              throw Exception ("Error parsing STL file " + Path::basename (path) + ": facet ended with " + str(vertex_index) + " vertices");
            triangles.push_back ( std::vector<uint32_t> { uint32_t(vertices.size()-3), uint32_t(vertices.size()-2), uint32_t(vertices.size()-1) } );
            vertex_index = 0;
            const Point<float> computed_normal = calc_normal (triangles.back());
            if (computed_normal.dot (normal) < 0.0)
              warn_right_hand_rule = true;
            if (std::abs (computed_normal.dot (normal)) < 0.99)
              warn_nonstandard_normals = true;
          } else {
            throw Exception ("Error parsing STL file " + Path::basename (path) + ": unknown key (" + line + ")");
          }
        }
        if (inside_facet)
          throw Exception ("Error parsing STL file " + Path::basename (path) + ": Failed to close facet");
        if (inside_loop)
          throw Exception ("Error parsing STL file " + Path::basename (path) + ": Failed to close loop");
        if (vertex_index)
          throw Exception ("Error parsing STL file " + Path::basename (path) + ": Failed to complete triangle");
      }

      if (warn_right_hand_rule)
        WARN ("File " + Path::basename (path) + " does not strictly conform to the right-hand rule");
      if (warn_nonstandard_normals)
        WARN ("File " + Path::basename (path) + " contains non-standard normals, which will be ignored");

      verify_data();
    }

    void Mesh::load_obj (const std::string& path)
    {
      std::ifstream in (path.c_str(), std::ios_base::in);
      if (!in)
        throw Exception ("Error opening input file!");
      std::string line;
      std::string group, object;
      int counter = -1;
      while (std::getline (in, line)) {
        ++counter;
        if (!line.size()) continue;
        if (line[0] == '#') continue;
        const size_t divider = line.find_first_of (' ');
        const std::string prefix (line.substr (0, divider));
        std::string data (line.substr (divider+1, line.npos));
        if (prefix == "v") {
          float values[4];
          sscanf (data.c_str(), "%f %f %f %f", &values[0], &values[1], &values[2], &values[3]);
          vertices.push_back (Vertex (values[0], values[1], values[2]));
        } else if (prefix == "vt") {
          // Texture data; do nothing
        } else if (prefix == "vn") {
          float values[3];
          sscanf (data.c_str(), "%f %f %f", &values[0], &values[1], &values[2]);
          normals.push_back (Vertex (values[0], values[1], values[2]));
        } else if (prefix == "vp") {
          // Parameter space vertices; do nothing
        } else if (prefix == "f") {
          // Parse face information
          // Need to handle:
          // * Either 3 or 4 vertices - write to either triangles or quads
          // * Vertices only, vertices & texture coordinates, vertices & normals, all 3
          std::vector<std::string> elements;
          do {
            const size_t first_space = data.find_first_of (' ');
            if (first_space == data.npos) {
              if (std::isalnum (data[0]))
                elements.push_back (data);
              data.clear();
            } else {
              elements.push_back (data.substr (0, first_space));
              data = data.substr (first_space+1);
            }
          } while (data.size());
          if (elements.size() != 3 && elements.size() != 4)
            throw Exception ("Malformed face information in input OBJ file (face with neither 3 nor 4 vertices; line " + str(counter) + ")");
          std::vector<FaceData> face_data;
          size_t values_per_element = 0;
          for (std::vector<std::string>::iterator i = elements.begin(); i != elements.end(); ++i) {
            FaceData temp;
            temp.vertex = 0; temp.texture = 0; temp.normal = 0;
            const size_t first_slash = i->find_first_of ('/');
            // OBJ format counts from 1 - therefore need to decrement
            temp.vertex = to<uint32_t> (i->substr (0, first_slash)) - 1;
            size_t this_values_count = 0;
            if (first_slash == i->npos) {
              this_values_count = 1;
            } else {
              const size_t last_slash = i->find_last_of ('/');
              if (last_slash == first_slash) {
                temp.texture = to<uint32_t> (i->substr (last_slash+1)) - 1;
                this_values_count = 2;
              } else {
                temp.texture = to<uint32_t> (i->substr (first_slash, last_slash)) - 1;
                temp.normal = to<uint32_t> (i->substr (last_slash+1)) - 1;
                this_values_count = 3;
              }
            }
            if (!values_per_element)
              values_per_element = this_values_count;
            else if (values_per_element != this_values_count)
              throw Exception ("Malformed face information in input OBJ file (inconsistent vertex / texture / normal detail); line " + str(counter));
            face_data.push_back (temp);
          }
          if (face_data.size() == 3) {
            std::vector<uint32_t> temp { face_data[0].vertex, face_data[1].vertex, face_data[2].vertex };
            triangles.push_back (Triangle (temp));
          } else {
            std::vector<uint32_t> temp { face_data[0].vertex, face_data[1].vertex, face_data[2].vertex, face_data[3].vertex };
            quads.push_back (Quad (temp));
          }
          // The OBJ format allows defining different vertex-based normals for different faces that reference the same vertex
          // This isn't consistent with the internal storage mechanism used in the Mesh class, and isn't really a feature
          //   worth providing support for in this context.
          // Therefore, just ignore this data
        } else if (prefix == "g") {
          //if (!group.size())
          //  group = data;
          //else
          //  throw Exception ("Multiple groups in input OBJ file");
          group = data;
        } else if (prefix == "o") {
          if (!object.size())
            object = data;
          else
            throw Exception ("Multiple objects in input OBJ file");
        } // Do nothing for all other prefixes
      }

      if (object.size())
        name = object;

      verify_data();
    }





    void Mesh::save_vtk (const std::string& path, const bool binary) const
    {
      File::OFStream out (path, std::ios_base::out);
      out << "# vtk DataFile Version 1.0\n";
      out << "\n";
      if (binary)
        out << "BINARY\n";
      else
        out << "ASCII\n";
      out << "DATASET POLYDATA\n";

      ProgressBar progress ("writing mesh to file... ", vertices.size() + triangles.size() + quads.size());
      if (binary) {

        // FIXME Binary VTK output _still_ not working (crashes ParaView)
        // Can however export as binary then -reconvert to ASCII and al is well...?
        // Changed to big-endian output, doesn't seem to have fixed...

        out.close();
        out.open (path, std::ios_base::out | std::ios_base::app | std::ios_base::binary);
        const std::string points_header ("POINTS " + str(vertices.size()) + " float\n");
        out.write (points_header.c_str(), points_header.size());
        for (VertexList::const_iterator i = vertices.begin(); i != vertices.end(); ++i) {
          //float temp[3];
          //for (size_t id = 0; id != 3; ++id)
          //  MR::putBE ((*i)[id], &temp[id]);
          const float temp[3] { (*i)[0], (*i)[1], (*i)[2] };
          out.write (reinterpret_cast<const char*>(temp), 3 * sizeof(float));
          ++progress;
        }
        const std::string polygons_header ("POLYGONS " + str(triangles.size() + quads.size()) + " " + str(4*triangles.size() + 5*quads.size()) + "\n");
        out.write (polygons_header.c_str(), polygons_header.size());
        const uint32_t num_points_triangle = 3;
        for (TriangleList::const_iterator i = triangles.begin(); i != triangles.end(); ++i) {
          out.write (reinterpret_cast<const char*>(&num_points_triangle), sizeof(uint32_t));
          //uint32_t temp[3];
          //for (size_t id = 0; id != 3; ++id)
          //  MR::putBE ((*i)[id], &temp[id]);
          const uint32_t temp[3] { (*i)[0], (*i)[1], (*i)[2] };
          out.write (reinterpret_cast<const char*>(temp), 3 * sizeof(uint32_t));
          ++progress;
        }
        const uint32_t num_points_quad = 4;
        for (QuadList::const_iterator i = quads.begin(); i != quads.end(); ++i) {
          out.write (reinterpret_cast<const char*>(&num_points_quad), sizeof(uint32_t));
          //uint32_t temp[4];
          //for (size_t id = 0; id != 4; ++id)
          //  MR::putBE ((*i)[id], &temp[id]);
          const uint32_t temp[4] { (*i)[0], (*i)[1], (*i)[2], (*i)[3] };
          out.write (reinterpret_cast<const char*>(temp), 4 * sizeof(uint32_t));
          ++progress;
        }

      } else {

        out << "POINTS " << str(vertices.size()) << " float\n";
        for (VertexList::const_iterator i = vertices.begin(); i != vertices.end(); ++i) {
          out << str((*i)[0]) << " " << str((*i)[1]) << " " << str((*i)[2]) << "\n";
          ++progress;
        }
        out << "POLYGONS " + str(triangles.size() + quads.size()) + " " + str(4*triangles.size() + 5*quads.size()) + "\n";
        for (TriangleList::const_iterator i = triangles.begin(); i != triangles.end(); ++i) {
          out << "3 " << str((*i)[0]) << " " << str((*i)[1]) << " " << str((*i)[2]) << "\n";
          ++progress;
        }
        for (QuadList::const_iterator i = quads.begin(); i != quads.end(); ++i) {
          out << "4 " << str((*i)[0]) << " " << str((*i)[1]) << " " << str((*i)[2]) << " " << str((*i)[3]) << "\n";
          ++progress;
        }

      }
    }



    void Mesh::save_stl (const std::string& path, const bool binary) const
    {
      if (quads.size())
          throw Exception ("STL binary file format does not support quads; only triangles");

      ProgressBar progress ("writing mesh to file... ", triangles.size());

      if (binary) {

        File::OFStream out (path, std::ios_base::binary | std::ios_base::out);
        const std::string string = std::string ("mrtrix_version: ") + App::mrtrix_version;
        char header[80];
        strncpy (header, string.c_str(), 80);
        out.write (header, 80);
        const uint32_t count = triangles.size();
        out.write (reinterpret_cast<const char*>(&count), sizeof(uint32_t));
        const uint16_t attribute_byte_count = 0;
        for (TriangleList::const_iterator i = triangles.begin(); i != triangles.end(); ++i) {
          const Point<float> n (calc_normal (*i));
          out.write (reinterpret_cast<const char*>(&n), 3 * sizeof(float));
          for (size_t v = 0; v != 3; ++v) {
            const Point<float>& p (vertices[(*i)[v]]);
            out.write (reinterpret_cast<const char*>(&p), 3 * sizeof(float));
          }
          out.write (reinterpret_cast<const char*>(&attribute_byte_count), sizeof(uint16_t));
          ++progress;
        }

      } else {

        File::OFStream out (path);
        out << "solid \n";
        for (TriangleList::const_iterator i = triangles.begin(); i != triangles.end(); ++i) {
          const Point<float> n (calc_normal (*i));
          out << "facet normal " << str (n[0]) << " " << str (n[1]) << " " << str (n[2]) << "\n";
          out << "    outer loop\n";
          for (size_t v = 0; v != 3; ++v) {
            const Point<float> p (vertices[(*i)[v]]);
            out << "        vertex " << str (p[0]) << " " << str (p[1]) << " " << str (p[2]) << "\n";
          }
          out << "    endloop\n";
          out << "endfacet\n";
          ++progress;
        }
        out << "endsolid \n";

      }
    }



    void Mesh::save_obj (const std::string& path) const
    {
      File::OFStream out (path);
      out << "# mrtrix_version: " << App::mrtrix_version << "\n";
      out << "o " << name << "\n";
      for (VertexList::const_iterator v = vertices.begin(); v != vertices.end(); ++v)
        out << "v " << str((*v)[0]) << " " << str((*v)[1]) << " " << str((*v)[2]) << " 1.0\n";
      for (TriangleList::const_iterator t = triangles.begin(); t != triangles.end(); ++t)
        out << "f " << str((*t)[0]+1) << " " << str((*t)[1]+1) << " " << str((*t)[2]+1) << "\n";
      for (QuadList::const_iterator q = quads.begin(); q != quads.end(); ++q)
        out << "f " << str((*q)[0]+1) << " " << str((*q)[1]+1) << " " << str((*q)[2]+1) << " " << str((*q)[3]+1) << "\n";
    }





    void Mesh::verify_data() const
    {
      for (VertexList::const_iterator i = vertices.begin(); i != vertices.end(); ++i) {
        if (std::isnan ((*i)[0]) || std::isnan ((*i)[1]) || std::isnan ((*i)[2]))
          throw Exception ("NaN values in mesh vertex data");
      }
      for (TriangleList::const_iterator i = triangles.begin(); i != triangles.end(); ++i)
        for (size_t j = 0; j != 3; ++j)
          if ((*i)[j] >= vertices.size())
            throw Exception ("Mesh vertex index exceeds number of vertices read");
      for (QuadList::const_iterator i = quads.begin(); i != quads.end(); ++i)
        for (size_t j = 0; j != 4; ++j)
          if ((*i)[j] >= vertices.size())
            throw Exception ("Mesh vertex index exceeds number of vertices read");
    }



    void Mesh::load_triangle_vertices (VertexList& output, const size_t index) const
    {
      output.clear();
      for (size_t axis = 0; axis != 3; ++axis)
        output.push_back (vertices[triangles[index][axis]]);
    }

    void Mesh::load_quad_vertices (VertexList& output, const size_t index) const
    {
      output.clear();
      for (size_t axis = 0; axis != 4; ++axis)
        output.push_back (vertices[quads[index][axis]]);
    }





    Point<float> Mesh::calc_normal (const Triangle& in) const
    {
      return (((vertices[in[1]] - vertices[in[0]]).cross (vertices[in[2]] - vertices[in[1]])).normalise());
    }

    Point<float> Mesh::calc_normal (const Quad& in) const
    {
      return ((((vertices[in[1]] - vertices[in[0]]).cross (vertices[in[2]] - vertices[in[1]])).normalise()
             + ((vertices[in[2]] - vertices[in[0]]).cross (vertices[in[3]] - vertices[in[2]])).normalise())
             .normalise());
    }



    float Mesh::calc_area (const Triangle& in) const
    {
      return 0.5 * ((vertices[in[1]] - vertices[in[0]]).cross (vertices[in[2]] - vertices[in[0]]).norm());
    }
    float Mesh::calc_area (const Quad& in) const
    {
      const std::vector<uint32_t> v_one { in[0], in[1], in[2] };
      const std::vector<uint32_t> v_two { in[0], in[2], in[3] };
      const Triangle one (v_one), two (v_two);
      return (calc_area (one) + calc_area (two));
    }









    void MeshMulti::load (const std::string& path)
    {
      if (!Path::has_suffix (path, "obj") && !Path::has_suffix (path, "OBJ"))
        throw Exception ("Multiple meshes only supported by OBJ file format");

      std::ifstream in (path.c_str(), std::ios_base::in);
      if (!in)
        throw Exception ("Error opening input file!");
      std::string line;
      std::string object;
      int index = -1;
      int counter = -1;
      VertexList vertices;
      TriangleList triangles;
      QuadList quads;
      size_t vertex_index_offset = 1;
      while (std::getline (in, line)) {
        // TODO Functionalise most of the below
        ++counter;
        if (!line.size()) continue;
        if (line[0] == '#') continue;
        const size_t divider = line.find_first_of (' ');
        const std::string prefix (line.substr (0, divider));
        std::string data (line.substr (divider+1, line.npos));
        if (prefix == "v") {
          if (index < 0)
            throw Exception ("Malformed OBJ file; vertex outside object (line " + str(counter) + ")");
          float values[4];
          sscanf (data.c_str(), "%f %f %f %f", &values[0], &values[1], &values[2], &values[3]);
          vertices.push_back (Vertex (values[0], values[1], values[2]));
        } else if (prefix == "vt") {
        } else if (prefix == "vn") {
          //if (index < 0)
          //  throw Exception ("Malformed OBJ file; vertex normal outside object (line " + str(counter) + ")");
          //float values[3];
          //sscanf (data.c_str(), "%f %f %f", &values[0], &values[1], &values[2]);
          //normals.push_back (Vertex (values[0], values[1], values[2]));
        } else if (prefix == "vp") {
        } else if (prefix == "f") {
          if (index < 0)
            throw Exception ("Malformed OBJ file; face outside object (line " + str(counter) + ")");
          std::vector<std::string> elements;
          do {
            const size_t first_space = data.find_first_of (' ');
            if (first_space == data.npos) {
              if (std::isalnum (data[0]))
                elements.push_back (data);
              data.clear();
            } else {
              elements.push_back (data.substr (0, first_space));
              data = data.substr (first_space+1);
            }
          } while (data.size());
          if (elements.size() != 3 && elements.size() != 4)
            throw Exception ("Malformed face information in input OBJ file (face with neither 3 nor 4 vertices; line " + str(counter) + ")");
          std::vector<FaceData> face_data;
          size_t values_per_element = 0;
          for (std::vector<std::string>::iterator i = elements.begin(); i != elements.end(); ++i) {
            FaceData temp;
            temp.vertex = 0; temp.texture = 0; temp.normal = 0;
            const size_t first_slash = i->find_first_of ('/');
            temp.vertex = to<uint32_t> (i->substr (0, first_slash)) - vertex_index_offset;
            size_t this_values_count = 0;
            if (first_slash == i->npos) {
              this_values_count = 1;
            } else {
              const size_t last_slash = i->find_last_of ('/');
              if (last_slash == first_slash) {
                temp.texture = to<uint32_t> (i->substr (last_slash+1)) - vertex_index_offset;
                this_values_count = 2;
              } else {
                temp.texture = to<uint32_t> (i->substr (first_slash, last_slash)) - vertex_index_offset;
                temp.normal = to<uint32_t> (i->substr (last_slash+1)) - vertex_index_offset;
                this_values_count = 3;
              }
            }
            if (!values_per_element)
              values_per_element = this_values_count;
            else if (values_per_element != this_values_count)
              throw Exception ("Malformed face information in input OBJ file (inconsistent vertex / texture / normal detail); line " + str(counter));
            face_data.push_back (temp);
          }
          if (face_data.size() == 3) {
            std::vector<uint32_t> temp { face_data[0].vertex, face_data[1].vertex, face_data[2].vertex };
            triangles.push_back (Triangle (temp));
          } else {
            std::vector<uint32_t> temp { face_data[0].vertex, face_data[1].vertex, face_data[2].vertex, face_data[3].vertex };
            quads.push_back (Quad (temp));
          }
        } else if (prefix == "g") {
        } else if (prefix == "o") {
          // This is where this function differs from the standard OBJ load
          // Allow multiple objects; in fact explicitly expect them
          object = data;
          if (++index >= 0) {
            vertex_index_offset += vertices.size();
            // This should invoke a move, so no need to explicitly clear the local data
            Mesh temp;
            temp.load (vertices, triangles, quads);
            assert (vertices.empty());
            temp.set_name (object.size() ? object : str(index));
            push_back (temp);

          }
        }
      }

      if (vertices.size()) {
        ++index;
        Mesh temp;
        temp.load (vertices, triangles, quads);
        temp.set_name (object);
        push_back (temp);
      }
    }

    void MeshMulti::save (const std::string& path) const
    {
      if (!Path::has_suffix (path, "obj") && !Path::has_suffix (path, "OBJ"))
        throw Exception ("Multiple meshes only supported by OBJ file format");
      File::OFStream out (path);
      size_t offset = 1;
      out << "# mrtrix_version: " << App::mrtrix_version << "\n";
      for (const_iterator i = begin(); i != end(); ++i) {
        out << "o " << i->get_name() << "\n";
        for (VertexList::const_iterator v = i->vertices.begin(); v != i->vertices.end(); ++v)
          out << "v " << str((*v)[0]) << " " << str((*v)[1]) << " " << str((*v)[2]) << " 1.0\n";
        for (TriangleList::const_iterator t = i->triangles.begin(); t != i->triangles.end(); ++t)
          out << "f " << str((*t)[0]+offset) << " " << str((*t)[1]+offset) << " " << str((*t)[2]+offset) << "\n";
        for (QuadList::const_iterator q = i->quads.begin(); q != i->quads.end(); ++q)
          out << "f " << str((*q)[0]+offset) << " " << str((*q)[1]+offset) << " " << str((*q)[2]+offset) << " " << str((*q)[3]+offset) << "\n";
        offset += i->vertices.size();
      }
    }






  }
}


