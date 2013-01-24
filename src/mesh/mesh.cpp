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




    void Mesh::transform_fsl_to_image (const Image::Header& image)
    {

      // To make PVE calculations as simple as possible, transform all mesh points from FSL
      //   internal coordinate space to voxel-space coordinates

      // TODO Need to make this more robust

      for (VertexList::iterator v = vertices.begin(); v != vertices.end(); ++v) {
        (*v)[0] /= image.vox (0);
        (*v)[1] /= image.vox (1);
        (*v)[2] /= image.vox (2);
        (*v)[0]  = image.dim (0) - 1 - (*v)[0];
      }

    }





    // For initial segmentation of mesh - identify voxels on the mesh, inside & outside
    enum vox_mesh_t { UNDEFINED, ON_MESH, NOT_ON_MESH, OUTSIDE, INSIDE };



    void Mesh::output_pve_image (const Image::Header& template_image, const std::string& path) const
    {

      // 3D vector math should be accurate enough to only consider 6 nearest neighbours
      static const Point<int> adj_voxels[6] = { Point<int> (-1,  0,  0),
          Point<int> (+1,  0,  0),
          Point<int> ( 0, -1,  0),
          Point<int> ( 0, +1,  0),
          Point<int> ( 0,  0, -1),
          Point<int> ( 0,  0, +1)};

      // Offsets from the centre of a voxel to its corners
      static const Point<float> vox_cnr_offsets[8] = { Point<float> (-0.5, -0.5, -0.5),
          Point<float> (-0.5, -0.5, +0.5),
          Point<float> (-0.5, +0.5, -0.5),
          Point<float> (-0.5, +0.5, +0.5),
          Point<float> (+0.5, -0.5, -0.5),
          Point<float> (+0.5, +0.5, +0.5),
          Point<float> (+0.5, -0.5, -0.5),
          Point<float> (+0.5, +0.5, +0.5)};

      // Compute normals for polygons
      std::vector< Point<float> > normals;
      normals.reserve (polygons.size());
      for (PolygonList::const_iterator p = polygons.begin(); p != polygons.end(); ++p) {
        const Point<float> this_normal (((vertices[(*p)[1]] - vertices[(*p)[0]]).cross (vertices[(*p)[2]] - vertices[(*p)[1]])).normalise());
        normals.push_back (this_normal);
      }

      Image::Header H (template_image);
      H.datatype() = DataType::Float32; // Output intensity will range from 0.0 to 1.0

      // Create some memory to work with
      Image::BufferScratch<uint8_t> init_seg_data (H);
      Image::BufferScratch<uint8_t>::voxel_type init_seg (init_seg_data);

      std::vector< Point<int> > pv_voxels;

      // Step through the vertex list, flagging any voxels which intersect with them
      for (VertexList::const_iterator v = vertices.begin(); v != vertices.end(); ++v) {
        const Point<int> voxel (round ((*v)[0]), round ((*v)[1]), round ((*v)[2]));
        init_seg[0] = voxel[0];
        init_seg[1] = voxel[1];
        init_seg[2] = voxel[2];
        init_seg.value() = vox_mesh_t (ON_MESH);
        pv_voxels.push_back (voxel);
      }

      /*
  Image::Header H_onmesh (template_image);
  H_onmesh.datatype() = DataType::Bit;
  Image::Buffer<bool> onmesh_data ("onmesh.mif", H_onmesh);
  Image::Buffer<bool>::voxel_type v_onmesh (onmesh_data);
  Image::Loop loop;
  for (loop.start (init_seg, v_onmesh); loop.ok(); loop.next (init_seg, v_onmesh))
    v_onmesh.value() = (init_seg.value() == vox_mesh_t (ON_MESH));
       */

      // From here, expand the selection of PV-voxels to adjacent voxels in an iterative fashion,
      //   flagging any voxels which intersect with a polygon
      std::vector< Point<int> > to_expand (pv_voxels);
      do {
        const Point<int> centre_voxel = to_expand.back();
        to_expand.pop_back();

        for (size_t adj_vox_idx = 0; adj_vox_idx != 6; ++adj_vox_idx) {
          const Point<int> this_voxel (centre_voxel + adj_voxels[adj_vox_idx]);
          if (this_voxel[0] >= 0 && this_voxel[0] < init_seg.dim (0) && this_voxel[1] >= 0 && this_voxel[1] < init_seg.dim (1) && this_voxel[2] >= 0 && this_voxel[2] < init_seg.dim (2)) {
            init_seg[0] = this_voxel[0];
            init_seg[1] = this_voxel[1];
            init_seg[2] = this_voxel[2];
            if (init_seg.value() == vox_mesh_t (UNDEFINED)) { // Don't process a voxel more than once

              VertexList this_polygon;
              bool is_pv_voxel = false;
              for (size_t poly_idx = 0; poly_idx != polygons.size() && !is_pv_voxel; ++poly_idx) {
                load_vertices (this_polygon, poly_idx);

                // Early exit: For all three axes, must have at least one vertex smaller
                //   than +ve edge, and at least one larger than +ve edge
                // (Don't just test for a vertex within the voxel range - polygons may be larger than voxel size!)
                bool early_exit = false;
                for (size_t axis = 0; axis != 3 && !early_exit; ++axis) {
                  bool below_max = false, above_min = false;
                  for (VertexList::const_iterator v = this_polygon.begin(); v != this_polygon.end(); ++v) {
                    if ((*v)[axis] <= float(this_voxel[axis]) + 0.5)
                      below_max = true;
                    if ((*v)[axis] >= float(this_voxel[axis]) - 0.5)
                      above_min = true;
                  }
                  early_exit = !(below_max && above_min);
                }
                if (!early_exit) {

                  const Point<float> polygon_centre ((this_polygon[0] + this_polygon[1] + this_polygon[2]) * (1.0 / 3.0));

                  bool corner_outside = false, corner_inside = false;
                  for (size_t cnr_idx = 0; cnr_idx != 8; ++cnr_idx) {
                    const Point<float> corner (Point<float>(this_voxel) + vox_cnr_offsets[cnr_idx]);
                    const Point<float> diff (corner - polygon_centre);
                    if (diff.dot (normals[poly_idx]) >= 0.0)
                      corner_outside = true;
                    else
                      corner_inside = true;
                  }
                  is_pv_voxel = corner_outside && corner_inside;

                }
              }

              if (is_pv_voxel) {
                init_seg.value() = ON_MESH;
                pv_voxels.push_back (this_voxel);
                to_expand.push_back (this_voxel);
              } else {
                init_seg.value() = NOT_ON_MESH;
              }

            }

          }
        }

      } while (!to_expand.empty());

      /*
  Image::Header H_init (template_image);
  H_init.datatype() = DataType::UInt8;
  Image::Buffer<uint8_t> init_data ("init_seg.mif", H_init);
  Image::Buffer<uint8_t>::voxel_type v_init (init_data);
  Image::copy (init_seg, v_init);
       */

      // For those voxels which are not on the mesh, fill them as either inside or outside

      const Point<int> corner_voxels[8] = { Point<int> (                0,                 0,                 0),
          Point<int> (                0,                 0, template_image.dim (2) - 1),
          Point<int> (                0, template_image.dim (1) - 1,                 0),
          Point<int> (                0, template_image.dim (1) - 1, template_image.dim (2) - 1),
          Point<int> (template_image.dim (0) - 1,                 0,                 0),
          Point<int> (template_image.dim (0) - 1,                 0, template_image.dim (2) - 1),
          Point<int> (template_image.dim (0) - 1, template_image.dim (1) - 1,                 0),
          Point<int> (template_image.dim (0) - 1, template_image.dim (1) - 1, template_image.dim (2) - 1)};

      to_expand.push_back (corner_voxels[0]);
      init_seg[0] = corner_voxels[0][0];
      init_seg[1] = corner_voxels[0][1];
      init_seg[2] = corner_voxels[0][2];
      init_seg.value() = vox_mesh_t (OUTSIDE);
      do {
        const Point<int> centre_voxel (to_expand.back());
        to_expand.pop_back();

        for (size_t adj_vox_idx = 0; adj_vox_idx != 6; ++adj_vox_idx) {
          const Point<int> this_voxel (centre_voxel + adj_voxels[adj_vox_idx]);
          if (this_voxel[0] >= 0 && this_voxel[0] < init_seg.dim (0) && this_voxel[1] >= 0 && this_voxel[1] < init_seg.dim (1) && this_voxel[2] >= 0 && this_voxel[2] < init_seg.dim (2)) {

            init_seg[0] = this_voxel[0];
            init_seg[1] = this_voxel[1];
            init_seg[2] = this_voxel[2];
            if (init_seg.value() == vox_mesh_t (UNDEFINED) || init_seg.value() == vox_mesh_t (NOT_ON_MESH)) {
              init_seg.value() = vox_mesh_t (OUTSIDE);
              to_expand.push_back (this_voxel);
            }

          }
        }

      } while (!to_expand.empty());

      for (size_t cnr_idx = 0; cnr_idx != 8; ++cnr_idx) {
        init_seg[0] = corner_voxels[cnr_idx][0];
        init_seg[1] = corner_voxels[cnr_idx][1];
        init_seg[2] = corner_voxels[cnr_idx][2];
        if (init_seg.value() == vox_mesh_t (UNDEFINED))
          throw Exception ("Mesh is not bound within image field of view");
      }

      Image::Loop loop;
      for (loop.start (init_seg); loop.ok(); loop.next (init_seg)) {
        if (init_seg.value() == vox_mesh_t (UNDEFINED) || init_seg.value() == vox_mesh_t (NOT_ON_MESH))
          init_seg.value() = vox_mesh_t (INSIDE);
      }

      Image::BufferScratch<float> pve_est_data (H);
      Image::BufferScratch<float>::voxel_type pve_est (pve_est_data);

      for (loop.start (init_seg, pve_est); loop.ok(); loop.next (init_seg, pve_est)) {
        switch (init_seg.value()) {
          case vox_mesh_t (UNDEFINED):   throw Exception ("Poor filling of initial mesh estimate"); break;
          case vox_mesh_t (ON_MESH):     pve_est.value() = 0.5; break;
          case vox_mesh_t (NOT_ON_MESH): throw Exception ("Poor filling of initial mesh estimate"); break;
          case vox_mesh_t (OUTSIDE):     pve_est.value() = 0.0; break;
          case vox_mesh_t (INSIDE):      pve_est.value() = 1.0; break;
          default:                       throw Exception ("Poor filling of initial mesh estimate"); break;
        }
      }

      /*
  Image::Header H_est (template_image);
  H_est.datatype() = DataType::Float32;
  Image::Buffer<float> est_data ("estimate.mif", H_est);
  Image::Buffer<float>::voxel_type v_est (est_data);
  Image::copy (pve_est, v_est);
       */

      static const size_t pve_os_ratio = 10;

      // Obtain better partial volume estimates for those voxels that lie on the mesh
      for (std::vector< Point<int> >::const_iterator vox = pv_voxels.begin(); vox != pv_voxels.end(); ++vox) {
        pve_est[0] = (*vox)[0];
        pve_est[1] = (*vox)[1];
        pve_est[2] = (*vox)[2];

        // Generate a set of points within this voxel which need to be tested individually
        std::vector< Point<float> > to_test;
        to_test.reserve (Math::pow3 (pve_os_ratio));

        for (size_t x_idx = 0; x_idx != pve_os_ratio; ++x_idx) {
          const float x = (*vox)[0] - 0.5 + ((float(x_idx) + 0.5) / float(pve_os_ratio));
          for (size_t y_idx = 0; y_idx != pve_os_ratio; ++y_idx) {
            const float y = (*vox)[1] - 0.5 + ((float(y_idx) + 0.5) / float(pve_os_ratio));
            for (size_t z_idx = 0; z_idx != pve_os_ratio; ++z_idx) {
              const float z = (*vox)[2] - 0.5 + ((float(z_idx) + 0.5) / float(pve_os_ratio));
              to_test.push_back (Point<float> (x, y, z));
            }
          }
        }

        // Determine those polygons that should be considered during PV-estimation for this voxel
        std::vector<size_t> near_polys;
        for (size_t p = 0; p != polygons.size(); ++p) {

          VertexList v;
          load_vertices (v, p);

          const Polygon<3>& polygon (polygons[p]);
          bool consider_this_polygon = true;
          for (size_t axis = 0; axis != 3 && consider_this_polygon; ++axis) {
            bool below_max = false, above_min = false;
            for (size_t vertex_index = 0; vertex_index != 3; ++vertex_index) {
              const Vertex& vertex (vertices[polygon[vertex_index]]);
              if (vertex[axis] <= float((*vox)[axis]) + 0.5)
                below_max = true;
              if (vertex[axis] >= float((*vox)[axis]) - 0.5)
                above_min = true;
            }
            consider_this_polygon = below_max && above_min;
          }
          if (consider_this_polygon)
            near_polys.push_back (p);

        }

        if (near_polys.empty())
          throw Exception ("Voxel labelled as interacting with mesh, but no polygons found");

        int inside_mesh_count = 0;
        for (std::vector< Point<float> >::const_iterator i_p = to_test.begin(); i_p != to_test.end(); ++i_p) {
          const Point<float>& p (*i_p);

          float best_min_edge_distance = -INFINITY;
          bool best_result_inside = false;

          for (std::vector<size_t>::const_iterator polygon_index = near_polys.begin(); polygon_index != near_polys.end(); ++polygon_index) {
            const Point<float>& n (normals[*polygon_index]);

            VertexList v;
            load_vertices (v, *polygon_index);

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

        pve_est.value() = (float)inside_mesh_count / (float)Math::pow3 (pve_os_ratio);

      }

      // Finally, output the result
      Image::Buffer<float> out (path, H);
      Image::Buffer<float>::voxel_type v (out);
      for (loop.start (pve_est, v); loop.ok(); loop.next (pve_est, v))
        v.value() = pve_est.value();

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
        throw Exception ("Unsupported dataset type in .vtk file");

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



    void Mesh::load_vertices (VertexList& output, const size_t index) const
    {
      output.clear();
      for (size_t axis = 0; axis != 3; ++axis)
        output.push_back (vertices[polygons[index][axis]]);
    }



  }
}


