/* Copyright (c) 2008-2017 the MRtrix3 contributors.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, you can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * MRtrix is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * For more details, see http://www.mrtrix.org/.
 */


#ifndef __gui_mrview_tool_screen_capture_h__
#define __gui_mrview_tool_screen_capture_h__

#include <deque>

#include "gui/mrview/tool/base.h"
#include "gui/mrview/adjust_button.h"
#include "gui/mrview/spin_box.h"


namespace MR
{
  namespace GUI
  {

    namespace MRView
    {
      class AdjustButton;

      namespace Tool
      {


        class Capture : public Base
        { MEMALIGN(Capture)
          Q_OBJECT
          public:
            Capture (Dock* parent);
            virtual ~Capture() {}

            static void add_commandline_options (MR::App::OptionList& options);
            virtual bool process_commandline_option (const MR::App::ParsedOption& opt);

          private slots:
            void on_image_changed ();
            void on_rotation_type (int);
            void on_translation_type (int);
            void on_screen_capture ();
            void on_screen_preview ();
            void on_screen_stop ();
            void select_output_folder_slot();
            void on_output_update ();
            void on_restore_capture_state ();

          private:
            enum RotationType { World = 0, Eye, Image } rotation_type;
            QComboBox *rotation_type_combobox;
            AdjustButton *rotation_axis_x;
            AdjustButton *rotation_axis_y;
            AdjustButton *rotation_axis_z;
            AdjustButton *degrees_button;

            enum TranslationType { Voxel = 0, Scanner, Camera } translation_type;

            QComboBox* translation_type_combobox;
            AdjustButton *translate_x;
            AdjustButton *translate_y;
            AdjustButton *translate_z;

            SpinBox *target_volume;
            AdjustButton *FOV_multipler;
            SpinBox *start_index;
            SpinBox *frames;
            SpinBox *volume_axis;
            QLineEdit *prefix_textbox;
            QPushButton *folder_button;
            int axis;
            QDir* directory;

            bool is_playing;

            class CaptureState { MEMALIGN(CaptureState)
              public:
                Eigen::Quaternionf orientation;
                Eigen::Vector3f focus, target;
                float fov;
                size_t volume, volume_axis;
                size_t frame_index;
                int plane;
                CaptureState(const Eigen::Quaternionf& orientation,
                  const Eigen::Vector3f& focus, const Eigen::Vector3f& target, float fov,
                  size_t volume, size_t volume_axis,
                  size_t frame_index, int plane)
                  : orientation(orientation), focus(focus), target(target), fov(fov),
                    volume(volume), volume_axis(volume_axis),
                    frame_index(frame_index), plane(plane) {}
            };
            constexpr static size_t max_cache_size = 1;
            std::deque<CaptureState> cached_state;

            void run (bool with_capture);
            void cache_capture_state ();
        };

      }
    }
  }
}

#endif





