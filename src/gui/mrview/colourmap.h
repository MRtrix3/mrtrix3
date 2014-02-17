/*******************************************************************************
    Copyright (C) 2014 Brain Research Institute, Melbourne, Australia
    
    Permission is hereby granted under the Patent Licence Agreement between
    the BRI and Siemens AG from July 3rd, 2012, to Siemens AG obtaining a
    copy of this software and associated documentation files (the
    "Software"), to deal in the Software without restriction, including
    without limitation the rights to possess, use, develop, manufacture,
    import, offer for sale, market, sell, lease or otherwise distribute
    Products, and to permit persons to whom the Software is furnished to do
    so, subject to the following conditions:
    
    The above copyright notice and this permission notice shall be included
    in all copies or substantial portions of the Software.
    
    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
    OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
    MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
    IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
    CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
    TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
    SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

*******************************************************************************/


#ifndef __gui_mrview_colourmap_h__
#define __gui_mrview_colourmap_h__

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
            void render (const Projection& projection, const Displayable& object, int position, bool inverted);

          protected:
            GL::VertexBuffer VB;
            GL::VertexArrayObject VAO;
            GL::Shader::Program frame_program, program;
            size_t current_index;
            bool current_inverted;
            const GLfloat width, height, inset, text_offset;

            void setup (size_t index, bool inverted);
        };



      }
    }
  }
}

#endif




