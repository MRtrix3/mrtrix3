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

#ifndef __gui_mrview_colourbars_h__
#define __gui_mrview_colourbars_h__

#include "gui/opengl/gl.h"
#include "gui/opengl/shader.h"
#include "colourmap.h"

namespace MR
{
  namespace GUI
  {
    class Projection;

    namespace MRView
    {
      class Displayable;


      inline size_t colourmap_index_from_menu (size_t n)
      {
        if (ColourMap::maps[n].special)
          --n;
        return n;
      }




      class ColourBars { NOMEMALIGN
        public:

          enum Position {
            None = 0x00,
            Top = 0x01,
            Bottom = 0x02,
            Left = 0x04,
            Right = 0x08,
            TopLeft = Top | Left,
            TopRight = Top | Right,
            BottomLeft = Bottom | Left,
            BottomRight = Bottom | Right
          };

          ColourBars();

          void begin (Projection* projection,
              const Position position,
              const size_t ncolourbars) {
            current_position = position;
            current_projection = projection;
            current_count = ncolourbars;
            current_colourbar_index = 0;
          }

          void render (size_t colourmap, bool inverted,
              float local_min_value, float local_max_value,
              float global_min_value, float global_range,
              Eigen::Array3f colour = { NAN, NAN, NAN });

          void render (const Displayable& object, bool inverted);

          void end () {
            current_position = Position::None;
            current_projection = nullptr;
            current_count = 0;
            current_colourbar_index = 0;
          }

        protected:
          GL::VertexBuffer VB;
          GL::VertexArrayObject VAO;
          GL::Shader::Program frame_program, program;
          size_t current_colourmap_index;
          bool current_colourmap_inverted;
          const GLfloat width, height, inset, text_offset, colourbar_padding;

          void setup (size_t index, bool inverted);

        private:
          static size_t max_n_rows;
          Position current_position;
          Projection* current_projection;
          size_t current_count, current_colourbar_index;
      };



    }
  }
}

#endif




