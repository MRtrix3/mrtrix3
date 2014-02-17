#ifndef __gui_mrview_mode_slice_h__
#define __gui_mrview_mode_slice_h__

#include "app.h"
#include "gui/mrview/mode/base.h"

namespace MR
{
  namespace GUI
  {
    namespace MRView
    {
      namespace Mode
      {

        class Slice : public Base
        {
          public:
            Slice (Window& parent) :
              Base (parent, FocusContrast | MoveTarget | TiltRotate) { }
            virtual ~Slice ();

            virtual void paint (Projection& with_projection);

            class Shader : public Displayable::Shader {
              public:
                virtual std::string vertex_shader_source (const Displayable& object);
                virtual std::string fragment_shader_source (const Displayable& object);
            } slice_shader;

          protected:

            void draw_plane (int axis, Displayable::Shader& shader_program, Projection& with_projection);
        };

      }
    }
  }
}

#endif




