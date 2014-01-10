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


#include <ctime>


namespace MR
{
  namespace Mesh
  {




    void Mesh::transform_first_to_realspace (const Image::Header& H)
    {

      Image::Transform transform (H);

      for (VertexList::iterator v = vertices.begin(); v != vertices.end(); ++v) {
        (*v)[0] = ((H.dim(0)-1) * H.vox(0)) - (*v)[0];
        *v = transform.image2scanner (*v);
      }

    }



    void Mesh::transform_realspace_to_voxel (const Image::Header& H)
    {
      Image::Transform transform (H);
      for (VertexList::iterator v = vertices.begin(); v != vertices.end(); ++v)
        *v = transform.scanner2voxel (*v);
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
      std::vector< Point<float> > normals;
      normals.reserve (polygons.size());
      for (PolygonList::const_iterator p = polygons.begin(); p != polygons.end(); ++p) {
        const Point<float> this_normal (((vertices[(*p)[1]] - vertices[(*p)[0]]).cross (vertices[(*p)[2]] - vertices[(*p)[1]])).normalise());
        normals.push_back (this_normal);
      }

      // Create some memory to work with:
      // Stores a flag for each voxel as encoded in enum vox_mesh_t
      Image::BufferScratch<uint8_t> init_seg_data (H);
      Image::BufferScratch<uint8_t>::voxel_type init_seg (init_seg_data);

      // For every voxel, stores those polygons that may intersect the voxel
      typedef std::map< Point<int>, std::vector<size_t> > Vox2Poly;
      Vox2Poly voxel2poly;

      // Map each polygon to the underlying voxels
      for (size_t poly_index = 0; poly_index != polygons.size(); ++poly_index) {

        // Figure out the voxel extent of this polygon in three dimensions
        Point<int> lower_bound (H.dim(0)-1, H.dim(1)-1, H.dim(2)-1), upper_bound (0, 0, 0);
        VertexList this_poly_verts;
        load_polygon_vertices (this_poly_verts, poly_index);
        for (VertexList::const_iterator v = this_poly_verts.begin(); v != this_poly_verts.end(); ++v) {
          for (size_t axis = 0; axis != 3; ++axis) {
            const int this_axis_voxel = Math::round((*v)[axis]);
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
      for (loop.start (init_seg); loop.ok(); loop.next (init_seg)) {
        if (init_seg.value() == vox_mesh_t (UNDEFINED))
          init_seg.value() = vox_mesh_t (INSIDE);
      }
      ++progress;


      // Generate the initial estimated PVE image
      Image::BufferScratch<float> pve_est_data (H);
      Image::BufferScratch<float>::voxel_type pve_est (pve_est_data);

      for (loop.start (init_seg, pve_est); loop.ok(); loop.next (init_seg, pve_est)) {
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
            const Point<float>& n (normals[*polygon_index]);

            VertexList v;
            load_polygon_vertices (v, *polygon_index);

            // First: is it aligned with the normal?
            const Point<float> poly_centre ((v[0] + v[1] + v[2]) * (1.0 / 3.0));
            const Point<float> diff (p - poly_centre);
            const bool is_inside = (diff.dot (n) <= 0.0);

            // Second: does it project onto the polygon?
            const Point<float> p_on_plane (p - (n * (diff.dot (n))));

            const float min_edge_distance = minvalue ((p_on_plane - v[0]).dot ((v[2]-v[0]).cross (n)),
                                                      (p_on_plane - v[2]).dot ((v[1]-v[2]).cross (n)),
                                                      (p_on_plane - v[1]).dot ((v[0]-v[1]).cross (n)));

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

            polygons.reserve (num_polygons);
            int polygon_count = 0, element_count = 0;
            while (polygon_count < num_polygons && element_count < num_elements) {

              int vertex_count;
              if (is_ascii) {
                std::getline (in, line);
                sscanf (line.c_str(), "%u", &vertex_count);
              } else {
                in.read (reinterpret_cast<char*>(&vertex_count), sizeof (int));
              }

              if (vertex_count != 3)
                throw Exception ("Could not parse file \"" + path + "\";  only suppport 3-vertex polygons");

              Polygon<3> t;

              if (is_ascii) {
                line = line.substr (line.find (' ') + 1); // Strip the vertex count
                sscanf (line.c_str(), "%u %u %u", &t[0], &t[1], &t[2]);
              } else {
                for (size_t i = 0; i != 3; ++i)
                  in.read (reinterpret_cast<char*>(&t[i]), sizeof (int));
              }
              polygons.push_back (t);
              ++polygon_count;
              element_count += 4;

            }
            if (polygon_count != num_polygons || element_count != num_elements)
              throw Exception ("Incorrectly read polygon data from .vtk file \"" + path + "\"");

          } else {
            throw Exception ("Unsupported data \"" + line + "\" in .vtk file \"" + path + "\"");
          }
        }
      }

      verify_data();

    }



    void Mesh::verify_data() const
    {

      for (VertexList::const_iterator i = vertices.begin(); i != vertices.end(); ++i) {
        if (isnan ((*i)[0]) || isnan ((*i)[1]) || isnan ((*i)[2]))
          throw Exception ("NaN values in mesh vertex data");
      }


      for (PolygonList::const_iterator i = polygons.begin(); i != polygons.end(); ++i) {
        for (size_t j = 0; j != 3; ++j) {
          if ((*i)[j] < 0)
            throw Exception ("Negative vertex index in polygon data");
          if ((*i)[j] >= vertices.size())
            throw Exception ("Mesh vertex index exceeds number of vertices read");
        }
      }

    }



    void Mesh::load_polygon_vertices (VertexList& output, const size_t index) const
    {
      output.clear();
      for (size_t axis = 0; axis != 3; ++axis)
        output.push_back (vertices[polygons[index][axis]]);
    }



  }
}


