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

#include "file/config.h"
#include "gui/opengl/font.h"
#include "gui/mrview/colourbars.h"
#include "gui/mrview/displayable.h"
#include "gui/projection.h"

namespace MR
{
  namespace GUI
  {
    namespace MRView
    {
      //CONF option: MRViewMaxNumColourBarRows
      //CONF default: 3
      //CONF The maximal number of rows used to layout a collection of rendered colourbars
      //CONF Note, that all tool-specific colourbars will form a single collection.
      size_t ColourBars::max_n_rows = File::Config::get_int ("MRViewMaxNumColourBarRows", 3);




      ColourBars::ColourBars () :
        current_colourmap_index (0),
        current_colourmap_inverted (false),
        //CONF option: MRViewColourBarWidth
        //CONF default: 20
        //CONF The width of the colourbar in MRView, in pixels.
        width (MR::File::Config::get_float ("MRViewColourBarWidth", 20.0f)),
        //CONF option: MRViewColourBarHeight
        //CONF default: 100
        //CONF The height of the colourbar in MRView, in pixels.
        height (MR::File::Config::get_float ("MRViewColourBarHeight", 100.0f)),
        //CONF option: MRViewColourBarInset
        //CONF default: 20
        //CONF How far away from the edge of the main window to place the
        //CONF colourbar in MRView, in pixels.
        inset (MR::File::Config::get_float ("MRViewColourBarInset", 20.0f)),
        //CONF option: MRViewColourBarTextOffset
        //CONF default: 10
        //CONF How far away from the colourbar to place the associated text,
        //CONF in pixels.
        text_offset (MR::File::Config::get_float ("MRViewColourBarTextOffset", 10.0f)),
        //CONF option: MRViewColourBarHorizontalPadding
        //CONF default: 100
        //CONF The width in pixels between horizontally adjacent colour bars.
        colourbar_padding (MR::File::Config::get_float ("MRViewColourBarHorizontalPadding", 100.0f))
      {
        end ();
      }








      void ColourBars::setup (size_t index, bool inverted)
      {
        program.clear();
        frame_program.clear();

        std::string source =
          "layout(location=0) in vec3 data;\n"
          "uniform float scale_x, scale_y;\n"
          "out float amplitude;\n"
          "void main () {\n"
          "  gl_Position = vec4 (data.x*scale_x-1.0, data.y*scale_y-1.0, 0.0, 1.0);\n"
          "  amplitude = ";
        if (inverted)
          source += "1.0 - ";
        source += "data.z;\n"
          "}\n";

        GL::Shader::Vertex vertex_shader (source);

        std::string shader =
          "in float amplitude;\n"
          "out vec3 color;\n"
          "uniform vec3 colourmap_colour;\n"
          "void main () {\n"
          "  " + std::string(ColourMap::maps[index].glsl_mapping) +
          "}\n";

        GL::Shader::Fragment fragment_shader (shader);

        program.attach (vertex_shader);
        program.attach (fragment_shader);
        program.link();

        GL::Shader::Fragment frame_fragment_shader (
            "out vec3 color;\n"
            "void main () {\n"
            "  color = vec3(1.0, 1.0, 0.0);\n"
            "}\n");

        frame_program.attach (vertex_shader);
        frame_program.attach (frame_fragment_shader);
        frame_program.link();

        current_colourmap_index = index;
        current_colourmap_inverted = inverted;
      }



      void ColourBars::render (const Displayable& object, bool inverted)
      {
        render (object.colourmap, inverted, object.scaling_min (), object.scaling_max (),
            object.scaling_min (), object.display_range,
            Eigen::Array3f { object.colour[0] / 255.0f, object.colour[1] / 255.0f, object.colour[2] / 255.0f });
      }



      void ColourBars::render (size_t colourmap, bool inverted,
          float local_min_value, float local_max_value,
          float global_min_value, float global_range,
          Eigen::Array3f colour)
      {
        if (!current_position) return;
        if (ColourMap::maps[colourmap].special) return;

        if (!program || !frame_program || colourmap != current_colourmap_index || current_colourmap_inverted != inverted)
          setup (colourmap, inverted);

        if (!VB || !VAO) {
          VB.gen();
          VAO.gen();

          VB.bind (gl::ARRAY_BUFFER);
          VAO.bind();

          gl::EnableVertexAttribArray (0);
          gl::VertexAttribPointer (0, 3, gl::FLOAT, gl::FALSE_, 0, (void*)0);
        }
        else {
          VB.bind (gl::ARRAY_BUFFER);
          VAO.bind();
        }

        // Clamp the min/max fractions
        float max_frac = std::min(std::max(0.0f, (local_max_value - global_min_value) / global_range), 1.0f);
        float min_frac = std::min(std::max(0.0f, (local_min_value - global_min_value) / global_range), max_frac);

        int max_bars_per_row = std::max((int)std::ceil((float)(current_count) / max_n_rows), 1);
        int ncols = (int)std::ceil((float)current_count / max_bars_per_row);
        int column_index = current_colourbar_index % max_bars_per_row;
        int row_index = current_colourbar_index / max_bars_per_row;
        float scaled_width = width / max_bars_per_row;
        float scaled_height = height / ncols;

        GLfloat data[] = {
          0.0f,   0.0f,  min_frac,
          0.0f,   scaled_height, max_frac,
          scaled_width, scaled_height, max_frac,
          scaled_width, 0.0f,  min_frac
        };
        float x_offset = 0.0f, y_offset = 0.0f;
        int halign = -1;

        if (current_position & Position::Right) {
          x_offset = current_projection->width() - (max_bars_per_row - column_index) * (scaled_width + inset + colourbar_padding)
            + colourbar_padding;
          halign = 1;
        } else if (current_position & Position::Left)
          x_offset = column_index * (scaled_width + inset + colourbar_padding) + inset;
        if (current_position & Position::Top)
          y_offset = current_projection->height() - (row_index + 1) * (scaled_height + inset * 2) + inset;
        else
          y_offset = row_index * (scaled_height + inset * 2) + inset;

        data[0] += x_offset; data[1] += y_offset;
        data[3] += x_offset; data[4] += y_offset;
        data[6] += x_offset; data[7] += y_offset;
        data[9] += x_offset; data[10] += y_offset;

        gl::BufferData (gl::ARRAY_BUFFER, sizeof(data), data, gl::STREAM_DRAW);

        gl::DepthMask (gl::FALSE_);
        gl::LineWidth (1.0);
        gl::Disable (gl::BLEND);
        gl::Disable (gl::DEPTH_TEST);

        program.start();
        gl::Uniform1f (gl::GetUniformLocation (program, "scale_x"), 2.0f / current_projection->width());
        gl::Uniform1f (gl::GetUniformLocation (program, "scale_y"), 2.0f / current_projection->height());
        if (ColourMap::maps[colourmap].is_colour)
          gl::Uniform3fv (gl::GetUniformLocation (program, "colourmap_colour"), 1, &colour[0]);
        gl::DrawArrays (gl::TRIANGLE_FAN, 0, 4);
        program.stop();

        frame_program.start();
        gl::Uniform1f (gl::GetUniformLocation (frame_program, "scale_x"), 2.0f / current_projection->width());
        gl::Uniform1f (gl::GetUniformLocation (frame_program, "scale_y"), 2.0f / current_projection->height());
        gl::DrawArrays (gl::LINE_LOOP, 0, 4);
        frame_program.stop();

        current_projection->setup_render_text();
        int x = halign > 0 ? data[0] - text_offset : data[6] + text_offset;
        current_projection->render_text_align (x, data[1], str(local_min_value), halign, 0);
        current_projection->render_text_align (x, data[4], str(local_max_value), halign, 0);
        current_projection->done_render_text();

        gl::DepthMask (gl::TRUE_);

        current_colourbar_index++;
      }



    }
  }
}

