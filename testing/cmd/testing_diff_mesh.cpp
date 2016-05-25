/*
    Copyright 2008 Brain Research Institute, Melbourne, Australia

    Written by Robert E. Smith, 2015.

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

#include "command.h"
#include "progressbar.h"

#include "types.h"
#include "mesh/mesh.h"

using namespace MR;
using namespace App;

void usage ()
{
  AUTHOR = "Robert E. Smith (robert.smith@florey.edu.au)";

  DESCRIPTION
  + "compare two mesh files for differences, within specified tolerance. "
    "Note that vertex normals are currently not tested.";

  ARGUMENTS
  + Argument ("in1", "a mesh file.").type_file_in ()
  + Argument ("in2", "another mesh file.").type_file_in ()
  + Argument ("tolerance", "the maximum distance to consider acceptable").type_float (0.0);
}



void run ()
{
  const default_type dist_sq = Math::pow2 (default_type(argument[2]));

  MR::Mesh::MeshMulti multi_in1, multi_in2;

  // Read in the mesh data
  try {
    MR::Mesh::Mesh mesh (argument[0]);
    multi_in1.push_back (mesh);
  } catch (...) {
    multi_in1.load (argument[0]);
  }
  
  try {
    MR::Mesh::Mesh mesh (argument[1]);
    multi_in2.push_back (mesh);
  } catch (...) {
    multi_in2.load (argument[1]);
  }
  
  if (multi_in1.size() != multi_in2.size())
    throw Exception ("Mismatched number of mesh objects (" + str(multi_in1.size()) + " - " + str(multi_in2.size()) + ") - test FAILED");
  
  for (size_t mesh_index = 0; mesh_index != multi_in1.size(); ++mesh_index) {
  
    const MR::Mesh::Mesh& in1 (multi_in1[mesh_index]);
    const MR::Mesh::Mesh& in2 (multi_in2[mesh_index]);

  // Can't test this: Some formats have to duplicate the vertex positions
  //if (in1.num_vertices() != in2.num_vertices())
  //  throw Exception ("Mismatched vertex count (" + str(in1.num_vertices()) + " - " + str(in2.num_vertices()) + ") - test FAILED");
  if (in1.num_triangles() != in2.num_triangles())
    throw Exception ("Mismatched triangle count (" + str(in1.num_triangles()) + " - " + str(in2.num_triangles()) + ") - test FAILED");
  if (in1.num_quads() != in2.num_quads())
    throw Exception ("Mismatched quad count (" + str(in1.num_quads()) + " - " + str(in2.num_quads()) + ") - test FAILED");

  // For every triangle and quad in one file, there must be a matching triangle/quad in the other
  // Can't rely on a preserved order; need to look through the entire list for a triangle/quad for
  //   which every vertex has a corresponding vertex

  for (size_t i = 0; i != in1.num_triangles(); ++i) {
    // Explicitly load the vertex data
    std::array<Eigen::Vector3, 3> v1;
    for (size_t vertex = 0; vertex != 3; ++vertex)
      v1[vertex] = in1.vert(in1.tri(i)[vertex]);
    bool match_found = false;
    for (size_t j = 0; j != in2.num_triangles() && !match_found; ++j) {
      std::array<Eigen::Vector3, 3> v2;
      for (size_t vertex = 0; vertex != 3; ++vertex)
        v2[vertex] = in2.vert (in2.tri(j)[vertex]);
      bool all_vertices_matched = true;
      for (size_t a = 0; a != 3; ++a) {
        size_t b = 0;
        for (; b != 3; ++b) {
          if ((v1[a]-v2[b]).squaredNorm() < dist_sq)
            break;

        }
        if (b == 3)
          all_vertices_matched = false;
      }
      if (all_vertices_matched)
        match_found = true;
    }
    if (!match_found)
      throw Exception ("Unmatched triangle - test FAILED");
  }

  for (size_t i = 0; i != in1.num_quads(); ++i) {
    std::array<Eigen::Vector3, 4> v1;
    for (size_t vertex = 0; vertex != 4; ++vertex)
      v1[vertex] = in1.vert (in1.quad(i)[vertex]);
    bool match_found = false;
    for (size_t j = 0; j != in2.num_quads() && !match_found; ++j) {
      std::array<Eigen::Vector3, 4> v2;
      for (size_t vertex = 0; vertex != 4; ++vertex)
        v2[vertex] = in2.vert (in2.quad(j)[vertex]);
      bool all_vertices_matched = true;
      for (size_t a = 0; a != 4; ++a) {
        size_t b = 0;
        for (; b != 4; ++b) {
          if ((v1[a]-v2[b]).squaredNorm() < dist_sq)
            break;

        }
        if (b == 4)
          all_vertices_matched = false;
      }
      if (all_vertices_matched)
        match_found = true;
    }
    if (!match_found)
      throw Exception ("Unmatched quad - test FAILED");
  }

  }

  CONSOLE ("data checked OK");
}

