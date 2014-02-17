#ifndef __gui_dwi_render_window_h__
#define __gui_dwi_render_window_h__

#include "gui/opengl/gl.h"
#include "file/path.h"
#include "math/SH.h"

namespace MR
{
  namespace GUI
  {
    namespace Dialog
    {
      class Lighting;
    }
    namespace DWI
    {

      class RenderFrame;

      class Window : public QMainWindow
      {
          Q_OBJECT

        public:
          Window (bool is_response_coefs);
          void set_values (const std::string& filename);

        protected slots:
          void open_slot ();
          void close_slot ();
          void use_lighting_slot (bool is_checked);
          void show_axes_slot (bool is_checked);
          void hide_negative_lobes_slot (bool is_checked);
          void colour_by_direction_slot (bool is_checked);
          void normalise_slot (bool is_checked);
          void response_slot (bool is_checked);
          void previous_slot ();
          void next_slot ();
          void previous_10_slot ();
          void next_10_slot ();
          void lmax_slot ();
          void lod_slot ();
          void screenshot_slot ();
          void lmax_inc_slot ();
          void lmax_dec_slot ();
          void advanced_lighting_slot ();

        protected:
          RenderFrame* render_frame;
          Dialog::Lighting* lighting_dialog;
          QActionGroup* lod_group, *lmax_group, *screenshot_OS_group;
          QAction* response_action;

          std::string name;
          int current;
          Math::Matrix<float> values;
          bool  is_response;

          void set_values (int row);
      };


    }
  }
}


#endif

