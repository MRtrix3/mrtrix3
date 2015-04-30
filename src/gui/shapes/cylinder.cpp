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

      std::vector< Point<GLfloat> > vertices;

      // Want to be able to display using a single DrawElements call; not worth faffing about
      //   with combinations of GL_TRIANGLES and GL_TRIANGLE_FAN
      // Also need to figure out how to deal with shifting the second set of vertices
      // Might be easiest to do using a second array buffer; this will determine which of the
      //   two provided endpoints to use
      // Actually, can probably just use the Z-coordinate

      const size_t N = Math::pow2 (level_of_detail + 1);
      const float angle_multiplier = 2.0 * Math::pi / float(N);

      vertices.push_back (Point<GLfloat> (0.0f, 0.0f, 0.0f));
      for (size_t i = 0; i != N; ++i)
        vertices.push_back (Point<GLfloat> (std::cos (i * angle_multiplier), std::sin (i * angle_multiplier), 0.0f));

      vertices.push_back (Point<GLfloat> (0.0f, 0.0f, 1.0f));
      for (size_t i = 1; i <= N; ++i)
        vertices.push_back (Point<GLfloat> (vertices[i][0], vertices[i][1], 1.0f));

      std::vector< Point<GLuint> > indices;

      for (size_t i = 1; i <= N; ++i) {
        const size_t next_near = (i == N) ? 1 : i+1;
        const size_t far = i + N + 1;
        const size_t next_far  = (i == N) ? N+2 : far+1;
        // Triangles along the length of the cylinder
        indices.push_back (Point<GLuint> (i, far, next_far));
        indices.push_back (Point<GLuint> (i, next_far, next_near));
        // At the near face
        indices.push_back (Point<GLuint> (0, i, next_near));
        // At the far face
        indices.push_back (Point<GLuint> (N+1, next_far, far));
      }

      vertex_buffer.gen();
      vertex_buffer.bind (gl::ARRAY_BUFFER);
      gl::BufferData (gl::ARRAY_BUFFER, vertices.size()*sizeof(Point<GLfloat>), &vertices[0][0], gl::STATIC_DRAW);

      num_indices = 3*indices.size();
      index_buffer.gen();
      index_buffer.bind();
      gl::BufferData (gl::ELEMENT_ARRAY_BUFFER, indices.size()*sizeof(Point<GLuint>), &indices[0], gl::STATIC_DRAW);

    }



    }
  }
}




