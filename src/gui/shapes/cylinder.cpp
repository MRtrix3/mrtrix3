/*
 * Copyright (c) 2008-2016 the MRtrix3 contributors
 * 
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/
 * 
 * MRtrix is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * 
 * For more details, see www.mrtrix.org
 * 
 */

#include "gui/shapes/cylinder.h"

#include <vector>

#include "math/math.h"

namespace MR
{
  namespace GUI
  {
    namespace Shapes
    {




    void Cylinder::LOD (const size_t level_of_detail)
    {

      vector<Eigen::Vector3f> vertices, normals;
      vector<Eigen::Array3i> indices;

      // Want to be able to display using a single DrawElements call; not worth faffing about
      //   with combinations of GL_TRIANGLES and GL_TRIANGLE_FAN

      // Want to do appropriate lighting on the cylinder, but without
      //   toggling between flat and interpolated normals
      // To do this, just duplicate the vertices, and store the normals manually
      //   (normals will also need to be rotated in the vertex shader)

      const int N = int(Math::pow2 (level_of_detail + 1));
      const float angle_multiplier = 2.0 * Math::pi / float(N);

      // The near face
      vertices.push_back (Eigen::Vector3f { 0.0f, 0.0f, 0.0f });
      normals.push_back (Eigen::Vector3f { 0.0f, 0.0f, -1.0f });
      for (int i = 0; i != N; ++i) {
        vertices.push_back (Eigen::Vector3f { std::cos (i * angle_multiplier), std::sin (i * angle_multiplier), 0.0f });
        normals.push_back (Eigen::Vector3f { 0.0f, 0.0f, -1.0f });
        if (i == N-1)
          indices.push_back (Eigen::Array3i { 0, i, 1 });
        else if (i)
          indices.push_back (Eigen::Array3i { 0, i, i+1 });
      }

      // The far face
      const int far_face_centre_index = int(vertices.size());
      vertices.push_back (Eigen::Vector3f { 0.0f, 0.0f, 1.0f });
      normals.push_back (Eigen::Vector3f { 0.0f, 0.0f, 1.0f });
      for (int i = 0; i != N; ++i) {
        vertices.push_back (Eigen::Vector3f { std::cos (i * angle_multiplier), std::sin (i * angle_multiplier), 1.0f });
        normals.push_back (Eigen::Vector3f { 0.0f, 0.0f, 1.0f });
        if (i == N-1)
          indices.push_back (Eigen::Array3i { far_face_centre_index, far_face_centre_index+1, far_face_centre_index+i });
        else if (i)
          indices.push_back (Eigen::Array3i { far_face_centre_index, far_face_centre_index+i+1, far_face_centre_index+i });
      }

      // The length of the cylinder
      vertices.push_back (Eigen::Vector3f { 1.0f, 0.0f, 0.0f });
      normals.push_back (Eigen::Vector3f { 1.0f, 0.0f, 0.0f });
      vertices.push_back (Eigen::Vector3f { 1.0f, 0.0f, 1.0f });
      normals.push_back (Eigen::Vector3f { 1.0f, 0.0f, 0.0f });
      for (int i = 1; i <= N; ++i) {
        const int V = vertices.size();
        vertices.push_back (Eigen::Vector3f { std::cos (i * angle_multiplier), std::sin (i * angle_multiplier), 0.0f });
        normals.push_back (Eigen::Vector3f { std::cos (i * angle_multiplier), std::sin (i * angle_multiplier), 0.0f });
        vertices.push_back (Eigen::Vector3f { std::cos (i * angle_multiplier), std::sin (i * angle_multiplier), 1.0f });
        normals.push_back (Eigen::Vector3f { std::cos (i * angle_multiplier), std::sin (i * angle_multiplier), 0.0f });
        indices.push_back (Eigen::Array3i { V-2, V-1, V });
        indices.push_back (Eigen::Array3i { V, V-1, V+1 });
      }

      vertex_buffer.gen();
      vertex_buffer.bind (gl::ARRAY_BUFFER);
      gl::BufferData (gl::ARRAY_BUFFER, vertices.size()*sizeof(Eigen::Vector3f), &vertices[0][0], gl::STATIC_DRAW);

      normal_buffer.gen();
      normal_buffer.bind (gl::ARRAY_BUFFER);
      gl::BufferData (gl::ARRAY_BUFFER, normals.size()*sizeof(Eigen::Vector3f), &normals[0][0], gl::STATIC_DRAW);

      num_indices = 3*indices.size();
      index_buffer.gen();
      index_buffer.bind();
      gl::BufferData (gl::ELEMENT_ARRAY_BUFFER, indices.size()*sizeof(Eigen::Array3i), &indices[0], gl::STATIC_DRAW);

    }



    }
  }
}




