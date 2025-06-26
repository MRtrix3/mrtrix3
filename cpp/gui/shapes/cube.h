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

#pragma once

// glutils.h must be included before gl_core_3_3.h since it include Qt OpenGL headers
// which must be included before gl_core_3_3.h
// clang-format off
#include "opengl/glutils.h"
#include "opengl/gl_core_3_3.h"
// clang-format on

namespace MR::GUI::Shapes {

class Cube {
public:
  Cube() : num_indices(0) {}

  void generate();

  GL::VertexBuffer vertex_buffer, normals_buffer;
  GL::IndexBuffer index_buffer;
  size_t num_indices;
};

} // namespace MR::GUI::Shapes
