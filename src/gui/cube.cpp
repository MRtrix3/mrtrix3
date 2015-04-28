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

#include "gui/cube.h"

#define NUM_VERTICES 8
#define NUM_POLYGONS 12

namespace
{



  static const GLfloat vertices[NUM_VERTICES][3] = {
      {-0.5, -0.5, +0.5}, {+0.5, -0.5, +0.5}, {+0.5, +0.5, +0.5}, {-0.5, +0.5, +0.5},
      {-0.5, -0.5, -0.5}, {+0.5, -0.5, -0.5}, {+0.5, +0.5, -0.5}, {-0.5, +0.5, -0.5}
  };

  static const GLfloat normals[NUM_VERTICES][3] = {
      { 0,  0, +1}, {+1,  0,  0}, { 0, +1,  0}, {-1,  0,  0},
      { 0, -1,  0}, { 0,  0, -1}, { 0,  0,  0}, { 0,  0,  0}
  };

  static const GLuint polygons[NUM_POLYGONS][3] = {
      {0,1,2}, {0,2,3},
      {1,5,6}, {1,6,2},
      {2,6,7}, {2,7,3},
      {3,4,0}, {3,7,4},
      {4,5,1}, {4,1,0},
      {5,7,6}, {5,4,7}
  };

}



namespace MR
{
  namespace GUI
  {


    void Cube::generate()
    {
      vertex_buffer.gen();
      vertex_buffer.bind (gl::ARRAY_BUFFER);
      gl::BufferData (gl::ARRAY_BUFFER, 3 * NUM_VERTICES * sizeof(GLfloat), &vertices[0][0], gl::STATIC_DRAW);

      normals_buffer.gen();
      normals_buffer.bind (gl::ARRAY_BUFFER);
      gl::BufferData (gl::ARRAY_BUFFER, 3 * NUM_VERTICES * sizeof(GLfloat), &normals[0][0], gl::STATIC_DRAW);

      index_buffer.gen();
      index_buffer.bind();
      num_indices = 3 * NUM_POLYGONS * sizeof(GLuint);
      gl::BufferData (gl::ELEMENT_ARRAY_BUFFER, num_indices, &polygons[0], gl::STATIC_DRAW);
    }



  }
}




