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

#include "gui/shapes/cylinder.h"

#include <vector>

#include "point.h"


namespace MR
{
  namespace GUI
  {
    namespace Shapes
    {




    void Cylinder::LOD (const size_t level_of_detail)
    {

      std::vector< Point<GLfloat> > vertices, normals;
      std::vector< Point<GLuint> > indices;

      // Want to be able to display using a single DrawElements call; not worth faffing about
      //   with combinations of GL_TRIANGLES and GL_TRIANGLE_FAN

      // Want to do appropriate lighting on the cylinder, but without
      //   toggling between flat and interpolated normals
      // To do this, just duplicate the vertices, and store the normals manually
      //   (normals will also need to be rotated in the vertex shader)

      const size_t N = Math::pow2 (level_of_detail + 1);
      const float angle_multiplier = 2.0 * Math::pi / float(N);

      // The near face
      vertices.push_back (Point<GLfloat> (0.0f, 0.0f, 0.0f));
      normals.push_back (Point<GLfloat> (0.0f, 0.0f, -1.0f));
      for (size_t i = 0; i != N; ++i) {
        vertices.push_back (Point<GLfloat> (std::cos (i * angle_multiplier), std::sin (i * angle_multiplier), 0.0f));
        normals.push_back (Point<GLfloat> (0.0f, 0.0f, -1.0f));
        if (i == N-1)
          indices.push_back (Point<GLuint> (0, i, 1));
        else if (i)
          indices.push_back (Point<GLuint> (0, i, i+1));
      }

      // The far face
      const size_t far_face_centre_index = vertices.size();
      vertices.push_back (Point<GLfloat> (0.0f, 0.0f, 1.0f));
      normals.push_back (Point<GLfloat> (0.0f, 0.0f, 1.0f));
      for (size_t i = 0; i != N; ++i) {
        vertices.push_back (Point<GLfloat> (std::cos (i * angle_multiplier), std::sin (i * angle_multiplier), 1.0f));
        normals.push_back (Point<GLfloat> (0.0f, 0.0f, 1.0f));
        if (i == N-1)
          indices.push_back (Point<GLuint> (far_face_centre_index, far_face_centre_index+1, far_face_centre_index+i));
        else if (i)
          indices.push_back (Point<GLuint> (far_face_centre_index, far_face_centre_index+i+1, far_face_centre_index+i));
      }

      // The length of the cylinder
      vertices.push_back (Point<GLfloat> (1.0f, 0.0f, 0.0f));
      normals.push_back (Point<GLfloat> (1.0f, 0.0f, 0.0f));
      vertices.push_back (Point<GLfloat> (1.0f, 0.0f, 1.0f));
      normals.push_back (Point<GLfloat> (1.0f, 0.0f, 0.0f));
      for (size_t i = 1; i <= N; ++i) {
        const GLuint V = vertices.size();
        vertices.push_back (Point<GLfloat> (std::cos (i * angle_multiplier), std::sin (i * angle_multiplier), 0.0f));
        normals.push_back (Point<GLfloat> (std::cos (i * angle_multiplier), std::sin (i * angle_multiplier), 0.0f));
        vertices.push_back (Point<GLfloat> (std::cos (i * angle_multiplier), std::sin (i * angle_multiplier), 1.0f));
        normals.push_back (Point<GLfloat> (std::cos (i * angle_multiplier), std::sin (i * angle_multiplier), 0.0f));
        indices.push_back (Point<GLuint> (V-2, V-1, V));
        indices.push_back (Point<GLuint> (V, V-1, V+1));
      }

      vertex_buffer.gen();
      vertex_buffer.bind (gl::ARRAY_BUFFER);
      gl::BufferData (gl::ARRAY_BUFFER, vertices.size()*sizeof(Point<GLfloat>), &vertices[0][0], gl::STATIC_DRAW);

      normal_buffer.gen();
      normal_buffer.bind (gl::ARRAY_BUFFER);
      gl::BufferData (gl::ARRAY_BUFFER, normals.size()*sizeof(Point<GLfloat>), &normals[0][0], gl::STATIC_DRAW);

      num_indices = 3*indices.size();
      index_buffer.gen();
      index_buffer.bind();
      gl::BufferData (gl::ELEMENT_ARRAY_BUFFER, indices.size()*sizeof(Point<GLuint>), &indices[0], gl::STATIC_DRAW);

    }



    }
  }
}




