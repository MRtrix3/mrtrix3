/* Copyright (c) 2008-2019 the MRtrix3 contributors.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Covered Software is provided under this License on an "as is"
 * basis, without warranty of any kind, either expressed, implied, or
 * statutory, including, without limitation, warranties that the
 * Covered Software is free of defects, merchantable, fit for a
 * particular purpose or non-infringing.
 * See the Mozilla Public License v. 2.0 for more details.
 *
 * For more details, see http://www.mrtrix.org/.
 */

#ifndef __gui_mrview_tool_annotation_h__
#define __gui_mrview_tool_annotation_h__

#include "math/versor.h"

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


        class Annotation : public Base
        { MEMALIGN(Annotation)
          Q_OBJECT
          public:
            Annotation (Dock* parent);
            virtual ~Annotation() {}

            static void add_commandline_options (MR::App::OptionList& options);
            virtual bool process_commandline_option (const MR::App::ParsedOption& opt);

          private slots:
            void on_image_changed ();
            void select_annotation_slot();
            void load_annotation_slot();
            void on_annotation_update ();

          private:
            class Item;
            class Model;
            QPushButton *open_button;
            Model* annotation_list_model;
            QListView* annotation_list_view;
            int axis;

            // class AnnotationState { MEMALIGN(AnnotationState)
            //   public:
            //     Math::Versorf orientation;
            //     Eigen::Vector3f focus, target;
            //     float fov;
            //     size_t volume, volume_axis;
            //     int plane;
            //     AnnotationState(const Math::Versorf& orientation,
            //       const Eigen::Vector3f& focus, const Eigen::Vector3f& target, float fov,
            //       size_t volume, size_t volume_axis, int plane)
            //       : orientation(orientation), focus(focus), target(target), fov(fov),
            //         volume(volume), volume_axis(volume_axis), plane(plane) { }
            // };
            class Label { MEMALIGN(Label)
              public:
                Eigen::Vector3f centre;
                std::string label, description;
                Eigen::MatrixXf coordinates;
                Label(const Eigen::Vector3f& centre, const std::string& label, const std::string& description)
                  : centre(centre), label(label), description(description) { }
            };
            vector<Label> labels;

        };

      }
    }
  }
}

#endif





