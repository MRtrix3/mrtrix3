/* Copyright (c) 2008-2023 the MRtrix3 contributors.
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


#include "gui/crosshair.h"

#include "gui/projection.h"

namespace MR
{
  namespace GUI
  {



    void Crosshair::render (const Eigen::Vector3f& focus,
                            const ModelViewProjection& MVP) const
    {
      if (!VB || !VAO) {
        VB.gen();
        VAO.gen();

        VB.bind (gl::ARRAY_BUFFER);
        VAO.bind();

        gl::EnableVertexAttribArray (0);
        gl::VertexAttribPointer (0, 2, gl::FLOAT, gl::FALSE_, 0, nullptr);
      } else {
        VB.bind (gl::ARRAY_BUFFER);
        VAO.bind();
      }

      if (!program) {
        GL::Shader::Vertex vertex_shader (
            "layout(location=0) in vec2 pos;\n"
            "void main () {\n"
            "  gl_Position = vec4 (pos, 0.0, 1.0);\n"
            "}\n");
        GL::Shader::Fragment fragment_shader (
            "out vec4 color;\n"
            "void main () {\n"
            "  color = vec4 (0.5, 0.5, 0.0, 1.0);\n"
            "}\n");
        program.attach (vertex_shader);
        program.attach (fragment_shader);
        program.link();
      }

      Eigen::Vector3f F = MVP.model_to_screen (focus);
      F[0] = std::round (F[0] - MVP.x_position()) - 0.5f;
      F[1] = std::round (F[1] - MVP.y_position()) + 0.5f;

      F[0] = 2.0f * F[0] / MVP.width() - 1.0f;
      F[1] = 2.0f * F[1] / MVP.height() - 1.0f;

      GLfloat data [] = {
        F[0], -1.0f,
        F[0], 1.0f,
        -1.0f, F[1],
        1.0f, F[1]
      };
      gl::BufferData (gl::ARRAY_BUFFER, sizeof(data), data, gl::STATIC_DRAW);

      gl::DepthMask (gl::TRUE_);
      gl::Disable (gl::BLEND);
      gl::LineWidth (1.0);

      program.start();
      gl::DrawArrays (gl::LINES, 0, 4);
      program.stop();
    }



  }
}

