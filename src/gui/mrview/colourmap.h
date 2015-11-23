/*
   Copyright 2010 Brain Research Institute, Melbourne, Australia

   Written by J-Donald Tournier, 13/11/09.

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

#ifndef __gui_mrview_colourmap_h__
#define __gui_mrview_colourmap_h__

#include <functional>

#include "point.h"

#include "gui/opengl/gl.h"
#include "gui/opengl/shader.h"

namespace MR
{
  namespace GUI
  {
    class Projection;

    namespace MRView
    {
      class Displayable;

      namespace ColourMap
      {

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


        class Entry {
          public:

            typedef std::function< Point<float> (float) > basic_map_fn;

            Entry (const char* name, const char* glsl_mapping, basic_map_fn basic_mapping,
                const char* amplitude = NULL, bool special = false, bool is_colour = false) :
              name (name),
              glsl_mapping (glsl_mapping),
              basic_mapping (basic_mapping),
              amplitude (amplitude ? amplitude : default_amplitude), 
              special (special),
              is_colour (is_colour) { }

            const char* name;
            const char* glsl_mapping;
            basic_map_fn basic_mapping;
            const char* amplitude;
            bool special, is_colour;

            static const char* default_amplitude;


        };

        extern const Entry maps[];





        inline size_t num () {
          size_t n = 0;
          while (maps[n].name) ++n;
          return n;
        }


        inline size_t num_scalar () {
          size_t n = 0, i = 0;
          while (maps[i].name) {
            if (!maps[i].special)
              ++n;
            ++i;
          }
          return n;
        }

        inline size_t index (const std::string& name) {
          size_t n = 0;
          while (maps[n].name != name) ++n;
          return n;
        }


        inline size_t num_special () {
          size_t n = 0, i = 0;
          while (maps[i].name) {
            if (maps[i].special)
              ++n;
            ++i;
          }
          return n;
        }



        void create_menu (QWidget* parent, QActionGroup*& group, QMenu* menu, QAction** & actions, bool create_shortcuts = false, bool use_special = true);

        inline size_t from_menu (size_t n)
        {
          if (maps[n].special)
            --n;
          return n;
        }




        class Renderer {
          public:
            Renderer();
            void begin_render_colourbars (Projection* projection,
                                          const Position position,
                                          const size_t ncolourbars) {
              current_position = position;
              current_projection = projection;
              current_ncolourbars = ncolourbars;
              current_colourbar_index = 0;
            }

            void render (size_t colourmap, bool inverted,
                         float local_min_value, float local_max_value,
                         float global_min_value, float global_range,
                         Point<float> colour = Point<float>());

            void render (const Displayable& object, bool inverted);

            void end_render_colourbars () {
              current_position = Position::None;
              current_projection = nullptr;
              current_ncolourbars = 0;
              current_colourbar_index = 0;
            }

          protected:
            GL::VertexBuffer VB;
            GL::VertexArrayObject VAO;
            GL::Shader::Program frame_program, program;
            size_t current_index;
            bool current_inverted;
            const GLfloat width, height, inset, text_offset, colourbar_padding;

            void setup (size_t index, bool inverted);

          private:
            static size_t max_n_rows;
            Position current_position;
            Projection* current_projection;
            size_t current_ncolourbars, current_colourbar_index;
        };



      }
    }
  }
}

#endif




