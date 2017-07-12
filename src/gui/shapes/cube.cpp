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


#include "gui/shapes/cube.h"

namespace
{

/*

  // This set of cube data relies on using flat shading with a first vertex
  //   provoking convention to provide flat rendered faces
  // This may be more efficient in cases where many cubes are being drawn, but
  //   requires a lot of unnecessary code branching in cases where the geometry
  //   to be drawn can vary.
  // It has therefore been removed in favour of a larger scheme where vertices
  //   are simply duplicated with different normals in order to achieve the
  //   necessary flat shading.

#define NUM_VERTICES 8
#define NUM_POLYGONS 12

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

*/

#define NUM_VERTICES 24
#define NUM_POLYGONS 12

  static const GLfloat vertices[NUM_VERTICES][3] = {
      {-0.5, -0.5, -0.5}, {-0.5, -0.5, +0.5}, {-0.5, +0.5, -0.5}, {-0.5, +0.5, +0.5}, // -X face
      {+0.5, -0.5, -0.5}, {+0.5, -0.5, +0.5}, {+0.5, +0.5, -0.5}, {+0.5, +0.5, +0.5}, // +X face
      {-0.5, -0.5, -0.5}, {-0.5, -0.5, +0.5}, {+0.5, -0.5, -0.5}, {+0.5, -0.5, +0.5}, // -Y face
      {-0.5, +0.5, -0.5}, {-0.5, +0.5, +0.5}, {+0.5, +0.5, -0.5}, {+0.5, +0.5, +0.5}, // +Y face
      {-0.5, -0.5, -0.5}, {-0.5, +0.5, -0.5}, {+0.5, -0.5, -0.5}, {+0.5, +0.5, -0.5}, // -Z face
      {-0.5, -0.5, +0.5}, {-0.5, +0.5, +0.5}, {+0.5, -0.5, +0.5}, {+0.5, +0.5, +0.5}  // +Z face
  };

  static const GLfloat normals[NUM_VERTICES][3] = {
      {-1,  0,  0}, {-1,  0,  0}, {-1,  0,  0}, {-1,  0,  0}, // -X face
      {+1,  0,  0}, {+1,  0,  0}, {+1,  0,  0}, {+1,  0,  0}, // +X face
      { 0, -1,  0}, { 0, -1,  0}, { 0, -1,  0}, { 0, -1,  0}, // -Y face
      { 0, +1,  0}, { 0, +1,  0}, { 0, +1,  0}, { 0, +1,  0}, // +Y face
      { 0,  0, -1}, { 0,  0, -1}, { 0,  0, -1}, { 0,  0, -1}, // -Z face
      { 0,  0, +1}, { 0,  0, +1}, { 0,  0, +1}, { 0,  0, +1}, // +Z face
  };

  static const GLuint polygons[NUM_POLYGONS][3] = {
      { 0 , 1,  2}, { 2,  1,  3}, // -X face
      { 4,  6,  5}, { 5,  6,  7}, // +X face
      { 8, 10,  9}, { 9, 10, 11}, // -Y face
      {12, 13, 14}, {14, 13, 15}, // +Y face
      {16, 17, 18}, {18, 17, 19}, // -Z face
      {20, 22, 21}, {21, 22, 23}  // +Z face
  };

}



namespace MR
{
  namespace GUI
  {
    namespace Shapes
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
}




