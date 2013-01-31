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

#include "gui/opengl/gl.h"
#include "gui/opengl/shader.h"
#include "gui/projection.h"

namespace MR
{
  namespace GUI
  {
    namespace MRView
    {
      class Displayable;

      namespace ColourMap
      {

        class Entry {
          public:
            Entry (const char* name, const char* mapping, const char* amplitude = NULL, bool special = false) : 
              name (name),
              mapping (mapping), 
              amplitude (amplitude ? amplitude : default_amplitude), 
              special (special) { }

            const char* name;
            const char* mapping;
            const char* amplitude;
            bool special;

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



        inline size_t num_special () {
          size_t n = 0, i = 0;
          while (maps[i].name) {
            if (maps[i].special)
              ++n;
            ++i;
          }
          return n;
        }



        void create_menu (QWidget* parent, QActionGroup*& group, QMenu* menu, QAction** & actions, bool create_shortcuts = false);

        inline size_t from_menu (size_t n)
        {
          if (maps[n].special)
            --n;
          return n;
        }




        class Renderer {
          public:
            Renderer();
            ~Renderer();

            void render (const Projection& projection, const Displayable& object, int position);

          protected:
            GLuint VB, VAO;
            GL::Shader::Program frame_program, program;
            size_t current_index;
            const GLfloat width, height, inset, text_offset;

            void setup (size_t index);
        };



      }
    }
  }
}

#endif




