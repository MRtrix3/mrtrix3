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

#include <array>

#include "opengl/shader.h"

namespace MR::GUI::GL {

class Font {
public:
  Font(const QFont font) : metric(font), font(font) {}

  void initGL(bool with_shadow = true);

  const QFontMetrics metric;

  void start(int width, int height, float red, float green, float blue) const {
    assert(program);
    gl::Disable(gl::DEPTH_TEST);
    gl::DepthMask(gl::FALSE_);
    gl::Enable(gl::BLEND);
    gl::BlendEquation(gl::FUNC_ADD);
    gl::BlendFunc(gl::SRC_ALPHA, gl::ONE_MINUS_SRC_ALPHA);
    program.start();
    gl::Uniform1f(gl::GetUniformLocation(program, "scale_x"), 2.0 / width);
    gl::Uniform1f(gl::GetUniformLocation(program, "scale_y"), 2.0 / height);
    gl::Uniform1f(gl::GetUniformLocation(program, "red"), red);
    gl::Uniform1f(gl::GetUniformLocation(program, "green"), green);
    gl::Uniform1f(gl::GetUniformLocation(program, "blue"), blue);
  }

  void stop() const {
    program.stop();
    gl::DepthMask(gl::TRUE_);
    gl::Disable(gl::BLEND);
  }

  void render(std::string_view text, int x, int y) const;

protected:
  const QFont font;
  GL::Texture tex;
  std::array<GL::VertexBuffer, 2> vertex_buffer;
  GL::VertexArrayObject vertex_array_object;
  GL::Shader::Program program;
  std::array<int, 256> font_width;
  int font_height;
  std::array<float, 256> font_tex_pos;
  std::array<float, 256> font_tex_width;
};

} // namespace MR::GUI::GL
