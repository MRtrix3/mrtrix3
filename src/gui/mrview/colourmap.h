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




