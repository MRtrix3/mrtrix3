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





