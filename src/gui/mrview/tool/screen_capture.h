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


#ifndef __gui_mrview_tool_screen_capture_h__
#define __gui_mrview_tool_screen_capture_h__

#include "gui/mrview/tool/base.h"
#include "gui/mrview/adjust_button.h"

class QComboBox;

namespace MR
{
  namespace GUI
  {

    namespace MRView
    {
      class AdjustButton;

      namespace Tool
      {


        class ScreenCapture : public Base
        {
          Q_OBJECT
          public:
            ScreenCapture (Window& main_window, Dock* parent);
            virtual ~ScreenCapture() {}
            bool process_batch_command (const std::string& cmd, const std::string& args);

          private slots:
            void on_screen_capture ();
            bool select_output_folder_slot();
            void on_output_update ();

          private:

            AdjustButton *rotation_axis_x;
            AdjustButton *rotation_axis_y;
            AdjustButton *rotation_axis_z;
            AdjustButton *degrees_button;
            AdjustButton *translate_x;
            AdjustButton *translate_y;
            AdjustButton *translate_z;
            AdjustButton *FOV_multipler;
            QSpinBox *start_index;
            QSpinBox *frames;
            QLineEdit *prefix_textbox;
            QPushButton *folder_button;
            int axis;
            QDir* directory;

        };

      }
    }
  }
}

#endif





