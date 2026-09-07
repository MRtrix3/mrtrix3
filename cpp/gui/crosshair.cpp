/* Copyright (c) 2008-2026 the MRtrix3 contributors.
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

#include "crosshair.h"

#include <array>

#include "projection.h"

namespace MR::GUI {

void Crosshair::render(const Eigen::Vector3f &focus, const ModelViewProjection &MVP, float thickness) const {
  if (!VB || !VAO) {
    VB.gen();
    VAO.gen();

    VB.bind(gl::ARRAY_BUFFER);
    VAO.bind();

    gl::EnableVertexAttribArray(0);
    gl::VertexAttribPointer(0, 2, gl::FLOAT, gl::FALSE_, 0, nullptr);
  } else {
    VB.bind(gl::ARRAY_BUFFER);
    VAO.bind();
  }

  if (!program) {
    GL::Shader::Vertex vertex_shader("layout(location=0) in vec2 pos;\n"
                                     "void main () {\n"
                                     "  gl_Position = vec4 (pos, 0.0, 1.0);\n"
                                     "}\n");
    GL::Shader::Fragment fragment_shader("out vec4 color;\n"
                                         "void main () {\n"
                                         "  color = vec4 (0.5, 0.5, 0.0, 1.0);\n"
                                         "}\n");
    program.attach(vertex_shader);
    program.attach(fragment_shader);
    program.link();
  }

  Eigen::Vector3f F = MVP.model_to_screen(focus);
  F[0] = std::round(F[0] - MVP.x_position()) - 0.5F;
  F[1] = std::round(F[1] - MVP.y_position()) + 0.5F;

  F[0] = 2.0F * F[0] / MVP.width() - 1.0F;
  F[1] = 2.0F * F[1] / MVP.height() - 1.0F;

  // The two crosshair lines are drawn as thickness-scaled quads rather than GL_LINES:
  // OpenGL core profiles only guarantee a line width of one pixel, so gl::LineWidth() cannot be
  // relied upon to thicken the focus point in proportion to the super-sampling ratio.
  const float hx = thickness / MVP.width();  // half-thickness of the vertical line, in clip units
  const float hy = thickness / MVP.height(); // half-thickness of the horizontal line, in clip units
  const std::array<GLfloat, 24> data = {
      -1.0F,     F[1] - hy, 1.0F,      F[1] - hy, 1.0F,      F[1] + hy, // horizontal line, first triangle
      -1.0F,     F[1] - hy, 1.0F,      F[1] + hy, -1.0F,     F[1] + hy, // horizontal line, second triangle
      F[0] - hx, -1.0F,     F[0] + hx, -1.0F,     F[0] + hx, 1.0F,      // vertical line, first triangle
      F[0] - hx, -1.0F,     F[0] + hx, 1.0F,      F[0] - hx, 1.0F};     // vertical line, second triangle
  gl::BufferData(gl::ARRAY_BUFFER, sizeof(data), data.data(), gl::STATIC_DRAW);

  gl::DepthMask(gl::TRUE_);
  gl::Disable(gl::BLEND);

  program.start();
  gl::DrawArrays(gl::TRIANGLES, 0, 12);
  program.stop();
}

} // namespace MR::GUI
