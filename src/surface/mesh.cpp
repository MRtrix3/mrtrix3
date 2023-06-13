/* Copyright (c) 2008-2023 the MRtrix3 contributors.
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

#include "surface/mesh.h"

#include <ios>
#include <iostream>
#include <string>

#include "types.h"

#include "surface/freesurfer.h"
#include "surface/utils.h"


namespace MR
{
  namespace Surface
  {



    Mesh::Mesh (const std::string& path)
    {
      if (path.substr (path.size() - 4) == ".vtk" || path.substr (path.size() - 4) == ".VTK") {
        load_vtk (path);
      } else if (path.substr (path.size() - 4) == ".stl" || path.substr (path.size() - 4) == ".STL") {
        load_stl (path);
      } else if (path.substr (path.size() - 4) == ".obj" || path.substr (path.size() - 4) == ".OBJ") {
        load_obj (path);
      } else {
        try {
          load_fs (path);
        } catch (...) {
          clear();
          throw Exception ("Input surface mesh file not in supported format");
        }
      }
      name = Path::basename (path);
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




    void Mesh::calculate_normals()
    {
      normals.clear();
      normals.assign (vertices.size(), Vertex (0.0, 0.0, 0.0));
      for (TriangleList::const_iterator p = triangles.begin(); p != triangles.end(); ++p) {
        const Vertex this_normal = normal (*this, *p);
        for (size_t index = 0; index != 3; ++index)
          normals[(*p)[index]] += this_normal;
      }
      for (QuadList::const_iterator p = quads.begin(); p != quads.end(); ++p) {
        const Vertex this_normal = normal (*this, *p);
        for (size_t index = 0; index != 4; ++index)
          normals[(*p)[index]] += this_normal;
      }
      for (VertexList::iterator n = normals.begin(); n != normals.end(); ++n)
        n->normalize();
    }





    void Mesh::load_vtk (const std::string& path)
    {

      std::ifstream in (path.c_str(), std::ios_base::binary);
      if (!in)
        throw Exception ("Error opening input file!");

      std::string line;

      // First line: VTK version ID
      MR::getline (in, line);
      // Strip the version numbers
      line[23] = line[25] = 'x';
      // Verify that the line is correct
      if (line != "# vtk DataFile Version x.x")
        throw Exception ("Incorrect first line of .vtk file");

      // Second line: identifier
      MR::getline (in, line);

      // Third line: format of data
      MR::getline (in, line);
      bool is_ascii = false;
      if (line == "ASCII")
        is_ascii = true;
      else if (line != "BINARY")
        throw Exception ("unknown data format in .vtk data");

      // Fourth line: Data set type
      MR::getline (in, line);
      if (line.substr(0, 7) != "DATASET")
        throw Exception ("Error in definition of .vtk dataset");
      line = line.substr (8);
      if (line == "STRUCTURED_POINTS" || line == "STRUCTURED_GRID" || line == "UNSTRUCTURED_GRID" || line == "RECTILINEAR_GRID" || line == "FIELD")
        throw Exception ("Unsupported dataset type (" + line + ") in .vtk file");

      // From here, don't necessarily know which parts of the data will come first
      while (!in.eof()) {

        // Need to read line in either ASCII mode or in raw binary
        if (is_ascii) {
          MR::getline (in, line);
        } else {
          line.clear();
          char c = 0;
          do {
            in.read (&c, sizeof (char));
            if (isalnum (c) || c == ' ')
              line.push_back (c);
          } while (!in.eof() && (isalnum (c) || c == ' '));
        }

        if (line.size()) {
          if (line.substr (0, 6) == "POINTS") {

            line = line.substr (7);
            const size_t ws = line.find (' ');
            const int num_vertices = to<int> (line.substr (0, ws));
            line = line.substr (ws + 1);
            bool is_double = false;
            if (line.substr (0, 6) == "double")
              is_double = true;
            else if (line.substr (0, 5) != "float")
              throw Exception ("Error in reading binary .vtk file: Unsupported datatype (\"" + line + "\"");

            vertices.reserve (num_vertices);
            for (int i = 0; i != num_vertices; ++i) {

              Vertex v;
              if (is_ascii) {
                MR::getline (in, line);
                sscanf (line.c_str(), "%lf %lf %lf", &v[0], &v[1], &v[2]);
              } else {
                if (is_double) {
                  double data[3];
                  in.read (reinterpret_cast<char*>(&data[0]), 3 * sizeof (double));
                  v = { data[0], data[1], data[2] };
                } else {
                  float data[3];
                  in.read (reinterpret_cast<char*>(&data[0]), 3 * sizeof (float));
                  v = { data[0], data[1], data[2] };
                }
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
                MR::getline (in, line);
                sscanf (line.c_str(), "%u", &vertex_count);
              } else {
                in.read (reinterpret_cast<char*>(&vertex_count), sizeof (int));
              }

              if (vertex_count != 3 && vertex_count != 4)
                throw Exception ("Could not parse file \"" + path + "\";  only suppport 3- and 4-vertex polygons");

              vector<unsigned int> t (vertex_count, 0);

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

      try {
        verify_data();
      } catch(Exception& e) {
        throw Exception (e, "Error verifying surface data from VTK file \"" + path + "\"");
      }
    }



    void Mesh::load_stl (const std::string& path)
    {
      std::ifstream in (path.c_str(), std::ios_base::in);
      if (!in)
        throw Exception ("Error opening input file!");

      bool warn_right_hand_rule = false, warn_nonstandard_normals = false;

      char init[6];
      in.get (init, 6);
      init[5] = '\0';

      if (strncmp (init, "solid", 5)) {

        // File is stored as binary
        in.close();
        in.open (path.c_str(), std::ios_base::in | std::ios_base::binary);
        char header[80];
        in.read (header, 80);

        uint32_t count;
        in.read (reinterpret_cast<char*>(&count), sizeof(uint32_t));

        uint16_t attribute_byte_count;
        bool warn_attribute = false;

        Eigen::Vector3f vertex, normal;

        while (in.read (reinterpret_cast<char*>(normal.data()), 3 * sizeof(float))) {
          for (size_t index = 0; index != 3; ++index) {
            if (!in.read (reinterpret_cast<char*>(vertex.data()), 3 * sizeof(float)))
              throw Exception ("Error in parsing STL file");
            vertices.push_back (vertex.cast<default_type>());
          }
          in.read (reinterpret_cast<char*>(&attribute_byte_count), sizeof(uint16_t));
          if (attribute_byte_count)
            warn_attribute = true;

          triangles.push_back ( vector<uint32_t> { uint32_t(vertices.size()-3), uint32_t(vertices.size()-2), uint32_t(vertices.size()-1) } );
          const Eigen::Vector3d computed_normal = Surface::normal (*this, triangles.back());
          if (computed_normal.dot (normal.cast<default_type>()) < 0.0)
            warn_right_hand_rule = true;
          if (abs (computed_normal.dot (normal.cast<default_type>())) < 0.99)
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

        Vertex vertex, normal;

        std::string line;
        size_t vertex_index = 0;
        bool inside_solid = true, inside_facet = false, inside_loop = false;
        while (std::getline (in, line)) {
          // Strip leading whitespace
          line = line.substr (line.find_first_not_of (' '), line.npos);
          if (line.substr(0, 12) == "facet normal") {
            if (!inside_solid)
              throw Exception ("Error parsing STL file " + Path::basename (path) + ": facet outside solid");
            if (inside_facet)
              throw Exception ("Error parsing STL file " + Path::basename (path) + ": nested facets");
            inside_facet = true;
            line = line.substr (12);
            sscanf (line.c_str(), "%lf %lf %lf", &normal[0], &normal[1], &normal[2]);
          } else if (line.substr(0, 10) == "outer loop") {
            if (inside_loop)
              throw Exception ("Error parsing STL file " + Path::basename (path) + ": nested loops");
            if (!inside_facet)
              throw Exception ("Error parsing STL file " + Path::basename (path) + ": loop outside facet");
            inside_loop = true;
          } else if (line.substr(0, 6) == "vertex") {
            if (!inside_loop)
              throw Exception ("Error parsing STL file " + Path::basename (path) + ": vertex outside loop");
            if (!inside_facet)
              throw Exception ("Error parsing STL file " + Path::basename (path) + ": vertex outside facet");
            line = line.substr (6);
            sscanf (line.c_str(), "%lf %lf %lf", &vertex[0], &vertex[1], &vertex[2]);
            vertices.push_back (vertex);
            ++vertex_index;
          } else if (line.substr(0, 7) == "endloop") {
            if (!inside_loop)
              throw Exception ("Error parsing STL file " + Path::basename (path) + ": loop ending without start");
            if (!inside_facet)
              throw Exception ("Error parsing STL file " + Path::basename (path) + ": loop ending outside facet");
            inside_loop = false;
          } else if (line.substr(0, 8) == "endfacet") {
            if (inside_loop)
              throw Exception ("Error parsing STL file " + Path::basename (path) + ": facet ending inside loop");
            if (!inside_facet)
              throw Exception ("Error parsing STL file " + Path::basename (path) + ": facet ending without start");
            inside_facet = false;
            if (vertex_index != 3)
              throw Exception ("Error parsing STL file " + Path::basename (path) + ": facet ended with " + str(vertex_index) + " vertices");
            triangles.push_back ( vector<uint32_t> { uint32_t(vertices.size()-3), uint32_t(vertices.size()-2), uint32_t(vertices.size()-1) } );
            vertex_index = 0;
            const Eigen::Vector3d computed_normal = Surface::normal (*this, triangles.back());
            if (computed_normal.dot (normal) < 0.0)
              warn_right_hand_rule = true;
            if (abs (computed_normal.dot (normal)) < 0.99)
              warn_nonstandard_normals = true;
          } else if (line.substr(0, 8) == "endsolid") {
            if (inside_facet)
              throw Exception ("Error parsing STL file " + Path::basename (path) + ": solid ending inside facet");
            inside_solid = false;
          } else if (line.substr(0, 5) == "solid") {
            throw Exception ("Error parsing STL file " + Path::basename (path) + ": multiple solids in file");
          } else {
            throw Exception ("Error parsing STL file " + Path::basename (path) + ": unknown key (" + line + ")");
          }
        }
        if (inside_solid)
          throw Exception ("Error parsing STL file " + Path::basename (path) + ": Failed to close solid");
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

      try {
        verify_data();
      } catch(Exception& e) {
        throw Exception (e, "Error verifying surface data from STL file \"" + path + "\"");
      }
    }



    void Mesh::load_obj (const std::string& path)
    {

      struct FaceData {
          uint32_t vertex, texture, normal;
      };

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
          vector<std::string> elements;
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
          vector<FaceData> face_data;
          size_t values_per_element = 0;
          for (vector<std::string>::iterator i = elements.begin(); i != elements.end(); ++i) {
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
            vector<uint32_t> temp { face_data[0].vertex, face_data[1].vertex, face_data[2].vertex };
            triangles.push_back (Triangle (temp));
          } else {
            vector<uint32_t> temp { face_data[0].vertex, face_data[1].vertex, face_data[2].vertex, face_data[3].vertex };
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

      try {
        verify_data();
      } catch(Exception& e) {
        throw Exception (e, "Error verifying surface data from OBJ file \"" + path + "\"");
      }
    }


    void Mesh::load_fs (const std::string& path)
    {

      std::ifstream in (path.c_str(), std::ios_base::in | std::ios_base::binary);
      if (!in)
        throw Exception ("Error opening input file!");

      const int32_t magic_number = FreeSurfer::get_int24_BE (in);

      if (magic_number == FreeSurfer::triangle_file_magic_number) {

        std::string comment;
        std::getline (in, comment);
        const auto first_newline_offset = in.tellg();

        // Some FreeSurfer files will have a second comment line; others will not
        // Need to make honest attempt at both possible scenarios
        auto load_triangles = [&] () {
          const int32_t num_vertices = FreeSurfer::get_BE<int32_t> (in);
          if (num_vertices <= 0)
            throw Exception ("Error reading FreeSurfer file: Non-positive vertex count (" + str(num_vertices) + ")");
          const int32_t num_polygons = FreeSurfer::get_BE<int32_t> (in);
          if (num_polygons <= 0)
            throw Exception ("Error reading FreeSurfer file: Non-positive polygon count (" + str(num_polygons) + ")");
          if (num_polygons > 3*num_vertices)
            throw Exception ("Error reading FreeSurfer file: More polygons (" + str(num_polygons) + ") than triple the number of vertices (" + str(num_vertices) + ")");
          if (num_polygons < num_vertices / 3)
            throw Exception ("Error reading FreeSurfer file: Not enough polygons (" + str(num_polygons) + ") to use all vertices (" + str(num_vertices) + ")");
          try {
            vertices.reserve (num_vertices);
            triangles.reserve (num_polygons);
          } catch (std::bad_alloc&) {
            vertices.shrink_to_fit();
            triangles.shrink_to_fit();
            throw Exception ("Error reading FreeSurfer file: Memory allocation ("
                             + str(num_vertices) + " vertices, " + str(num_polygons) + " polygons = erroneous?)");
          }
          for (int32_t i = 0; i != num_vertices; ++i) {
            std::array<float, 3> temp;
            for (size_t axis = 0; axis != 3; ++axis)
              temp[axis] = FreeSurfer::get_BE<float> (in);
            if (!in.good())
              throw Exception ("Error reading FreeSurfer file: EOF reached after " + str(vertices.size()) + " of " + str(num_vertices) + " vertices");
            vertices.push_back (Vertex (temp[0], temp[1], temp[2]));
          }
          for (int32_t i = 0; i != num_polygons; ++i) {
            std::array<int32_t, 3> temp;
            for (size_t v = 0; v != 3; ++v)
              temp[v] = FreeSurfer::get_BE<int32_t> (in);
            if (!in.good())
              throw Exception ("Error reading FreeSurfer file: EOF reached after " + str(triangles.size()) + " of " + str(num_polygons) + " triangles");
            triangles.push_back (Triangle (temp));
          }
        };

        try {
          load_triangles();
        } catch (Exception& e_onecomment) {
          vertices.clear();
          triangles.clear();
          in.clear();
          in.seekg (first_newline_offset, std::ios_base::beg);
          std::string second_comment;
          std::getline (in, second_comment);
          try {
            load_triangles();
          } catch (Exception& e_twocomments) {
            Exception e ("Unable to read FreeSurfer file \"" + path + "\"");
            e.push_back ("Error if file header is one-line comment:");
            e.push_back (e_onecomment);
            e.push_back ("Error if file header is two-line comment:");
            e.push_back (e_twocomments);
            e.display();
            throw e;
          }
        }

      } else if (magic_number == FreeSurfer::quad_file_magic_number) {

        const int32_t num_vertices = FreeSurfer::get_int24_BE (in);
        const int32_t num_polygons = FreeSurfer::get_int24_BE (in);
        vertices.reserve (num_vertices);
        for (int32_t i = 0; i != num_vertices; ++i) {
          int16_t temp[3];
          for (size_t axis = 0; axis != 3; ++axis)
            temp[axis] = FreeSurfer::get_BE<int16_t> (in);
          vertices.push_back (Vertex (0.01 * temp[0], 0.01 * temp[1], 0.01 * temp[2]));
        }
        for (int32_t i = 0; i != num_polygons; ++i) {
          std::array<int32_t, 4> temp;
          for (size_t v = 0; v != 4; ++v)
            temp[v] = FreeSurfer::get_int24_BE (in);
          quads.push_back (Quad (temp));
        }

      } else {
        throw Exception ("File " + Path::basename (path) + " is not a FreeSurfer surface file");
      }

      try {
        verify_data();
      } catch(Exception& e) {
        throw Exception (e, "Error verifying surface data from FreeSurfer file \"" + path + "\"");
      }
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

      ProgressBar progress ("writing mesh to file", vertices.size() + triangles.size() + quads.size());
      if (binary) {

        // FIXME Binary VTK output _still_ not working (crashes ParaView)
        // Can however export as binary then -reconvert to ASCII and al is well...?
        // Changed to big-endian output, doesn't seem to have fixed...

        out.close();
        out.open (path, std::ios_base::out | std::ios_base::app | std::ios_base::binary);
        const bool is_double = (sizeof(default_type) == 8);
        const std::string str_datatype = is_double ? "double" : "float";
        const std::string points_header ("POINTS " + str(vertices.size()) + " " + str_datatype + "\n");
        out.write (points_header.c_str(), points_header.size());
        for (VertexList::const_iterator i = vertices.begin(); i != vertices.end(); ++i) {
          //float temp[3];
          //for (size_t id = 0; id != 3; ++id)
          //  MR::putBE ((*i)[id], &temp[id]);
          if (is_double) {
            const double temp[3] { double((*i)[0]), double((*i)[1]), double((*i)[2]) };
            out.write (reinterpret_cast<const char*>(temp), 3 * sizeof(double));
          } else {
            const float temp[3] { float((*i)[0]), float((*i)[1]), float((*i)[2]) };
            out.write (reinterpret_cast<const char*>(temp), 3 * sizeof(float));
          }
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

      ProgressBar progress ("writing mesh to file", triangles.size());

      if (binary) {

        File::OFStream out (path, std::ios_base::binary | std::ios_base::out);
        const std::string string = std::string ("mrtrix_version: ") + App::mrtrix_version;
        char header[80];
        strncpy (header, string.c_str(), 79);
        out.write (header, 80);
        const uint32_t count = triangles.size();
        out.write (reinterpret_cast<const char*>(&count), sizeof(uint32_t));
        const uint16_t attribute_byte_count = 0;
        for (TriangleList::const_iterator i = triangles.begin(); i != triangles.end(); ++i) {
          const Eigen::Vector3d n (normal (*this, *i));
          const float n_temp[3] { float(n[0]), float(n[1]), float(n[2]) };
          out.write (reinterpret_cast<const char*>(&n_temp[0]), 3 * sizeof(float));
          for (size_t v = 0; v != 3; ++v) {
            const Vertex& p (vertices[(*i)[v]]);
            const float p_temp[3] { float(p[0]), float(p[1]), float(p[2]) };
            out.write (reinterpret_cast<const char*>(&p_temp[0]), 3 * sizeof(float));
          }
          out.write (reinterpret_cast<const char*>(&attribute_byte_count), sizeof(uint16_t));
          ++progress;
        }

      } else {

        File::OFStream out (path);
        out << "solid \n";
        for (TriangleList::const_iterator i = triangles.begin(); i != triangles.end(); ++i) {
          const Eigen::Vector3d n (normal (*this, *i));
          out << "facet normal " << str (n[0]) << " " << str (n[1]) << " " << str (n[2]) << "\n";
          out << "    outer loop\n";
          for (size_t v = 0; v != 3; ++v) {
            const Vertex p (vertices[(*i)[v]]);
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
      out << "# " << App::command_history_string << "\n";
      out << "o " << name << "\n";
      for (VertexList::const_iterator v = vertices.begin(); v != vertices.end(); ++v)
        out << "v " << str((*v)[0]) << " " << str((*v)[1]) << " " << str((*v)[2]) << " 1.0\n";
      for (TriangleList::const_iterator t = triangles.begin(); t != triangles.end(); ++t)
        out << "f " << str((*t)[0]+1) << " " << str((*t)[1]+1) << " " << str((*t)[2]+1) << "\n";
      for (QuadList::const_iterator q = quads.begin(); q != quads.end(); ++q)
        out << "f " << str((*q)[0]+1) << " " << str((*q)[1]+1) << " " << str((*q)[2]+1) << " " << str((*q)[3]+1) << "\n";
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




  }
}
