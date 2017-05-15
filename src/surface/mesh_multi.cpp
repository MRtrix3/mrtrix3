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


#include "surface/mesh_multi.h"

#include <ios>
#include <iostream>



namespace MR
{
  namespace Surface
  {



    void MeshMulti::load (const std::string& path)
    {

      struct FaceData { NOMEMALIGN
          uint32_t vertex, texture, normal;
      };

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
            vector<uint32_t> temp { face_data[0].vertex, face_data[1].vertex, face_data[2].vertex };
            triangles.push_back (Triangle (temp));
          } else {
            vector<uint32_t> temp { face_data[0].vertex, face_data[1].vertex, face_data[2].vertex, face_data[3].vertex };
            quads.push_back (Quad (temp));
          }
        } else if (prefix == "g") {
        } else if (prefix == "o") {
          // This is where this function differs from the standard OBJ load
          // Allow multiple objects; in fact explicitly expect them
          if (index++ >= 0) {
            vertex_index_offset += vertices.size();
            Mesh temp;
            temp.load (std::move (vertices), std::move (triangles), std::move (quads));
            temp.set_name (object.size() ? object : str(index-1));
            push_back (temp);
          }
          object = data;
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


