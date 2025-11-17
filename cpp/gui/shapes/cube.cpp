/* Copyright (c) 2008-2025 the MRtrix3 contributors.
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

#include "shapes/cube.h"

namespace {

// clang-format off
constexpr std::array<GLfloat, 72> vertices{
    // -X face         //
    -0.5, -0.5, -0.5,  //
    -0.5, -0.5, +0.5,  //
    -0.5, +0.5, -0.5,  //
    -0.5, +0.5, +0.5,  //
    // +X face         //
    +0.5, -0.5, -0.5,  //
    +0.5, -0.5, +0.5,  //
    +0.5, +0.5, -0.5,  //
    +0.5, +0.5, +0.5,  //
    // -Y face         //
    -0.5, -0.5, -0.5,  //
    -0.5, -0.5, +0.5,  //
    +0.5, -0.5, -0.5,  //
    +0.5, -0.5, +0.5,  //
    // +Y face         //
    -0.5, +0.5, -0.5,  //
    -0.5, +0.5, +0.5,  //
    +0.5, +0.5, -0.5,  //
    +0.5, +0.5, +0.5,  //
    // -Z face         //
    -0.5, -0.5, -0.5,  //
    -0.5, +0.5, -0.5,  //
    +0.5, -0.5, -0.5,  //
    +0.5, +0.5, -0.5,  //
    // +Z face         //
    -0.5, -0.5, +0.5,  //
    -0.5, +0.5, +0.5,  //
    +0.5, -0.5, +0.5,  //
    +0.5, +0.5, +0.5}; //

constexpr std::array<GLfloat, 72> normals{
    // -X face //
    -1, 0, 0,  //
    -1, 0, 0,  //
    -1, 0, 0,  //
    -1, 0, 0,  //
    // +X face //
    +1, 0, 0,  //
    +1, 0, 0,  //
    +1, 0, 0,  //
    +1, 0, 0,  //
    // -Y face //
    0, -1, 0,  //
    0, -1, 0,  //
    0, -1, 0,  //
    0, -1, 0,  //
    // +Y face //
    0, +1, 0,  //
    0, +1, 0,  //
    0, +1, 0,  //
    0, +1, 0,  //
    // -Z face //
    0, 0, -1,  //
    0, 0, -1,  //
    0, 0, -1,  //
    0, 0, -1,  //
    // +Z face //
    0, 0, +1,  //
    0, 0, +1,  //
    0, 0, +1,  //
    0, 0, +1}; //

constexpr std::array<GLuint, 36> polygons{
    // -X face   //
    0, 1, 2,     //
    2, 1, 3,     //
    // +X face   //
    4, 6, 5,     //
    5, 6, 7,     //
    // -Y face   //
    8, 10, 9,    //
    9, 10, 11,   //
    // +Y face   //
    12, 13, 14,  //
    14, 13, 15,  //
    // -Z face   //
    16, 17, 18,  //
    18, 17, 19,  //
    // +Z face   //
    20, 22, 21,  //
    21, 22, 23}; //
// clang-format on
} // namespace

namespace MR::GUI::Shapes {

void Cube::generate() {
  vertex_buffer.gen();
  vertex_buffer.bind(gl::ARRAY_BUFFER);
  gl::BufferData(gl::ARRAY_BUFFER, vertices.size() * sizeof(GLfloat), vertices.data(), gl::STATIC_DRAW);

  normals_buffer.gen();
  normals_buffer.bind(gl::ARRAY_BUFFER);
  gl::BufferData(gl::ARRAY_BUFFER, normals.size() * sizeof(GLfloat), normals.data(), gl::STATIC_DRAW);

  index_buffer.gen();
  index_buffer.bind();
  num_indices = polygons.size();
  gl::BufferData(gl::ELEMENT_ARRAY_BUFFER, num_indices, polygons.data(), gl::STATIC_DRAW);
}

} // namespace MR::GUI::Shapes
