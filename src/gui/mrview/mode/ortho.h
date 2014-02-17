#ifndef __gui_mrview_mode_ortho_h__
#define __gui_mrview_mode_ortho_h__

#include "app.h"
#include "gui/mrview/mode/slice.h"

namespace MR
{
  namespace GUI
  {
    namespace MRView
    {
      namespace Mode
      {

        class Ortho : public Slice
        {
            Q_OBJECT

          public:
              Ortho (Window& parent) : 
                Slice (parent),
                projections (3, projection),
                current_plane (0) { }

            virtual void paint (Projection& projection);

            virtual void mouse_press_event ();
            virtual void slice_move_event (int x);
            virtual void panthrough_event ();
            virtual const Projection* get_current_projection () const;

          protected:
            std::vector<Projection> projections;
            int current_plane;
            GL::VertexBuffer frame_VB;
            GL::VertexArrayObject frame_VAO;
            GL::Shader::Program frame_program;
        };

      }
    }
  }
}

#endif





