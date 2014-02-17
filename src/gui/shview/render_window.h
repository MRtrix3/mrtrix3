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

