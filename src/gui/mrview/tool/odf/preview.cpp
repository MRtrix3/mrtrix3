/*
 * Copyright (c) 2008-2018 the MRtrix3 contributors.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, you can obtain one at http://mozilla.org/MPL/2.0/
 *
 * MRtrix3 is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * For more details, see http://www.mrtrix.org/
 */


#include "gui/mrview/tool/odf/preview.h"
#include "gui/dwi/render_frame.h"
#include "gui/mrview/window.h"
#include "gui/mrview/tool/odf/odf.h"
#include "gui/mrview/mode/base.h"

namespace MR
{
  namespace GUI
  {
    namespace MRView
    {
      namespace Tool
      {




        ODF_Preview::RenderFrame::RenderFrame (QWidget* parent) :
            DWI::RenderFrame (parent) {
          setMinimumSize (300, 300);    
        }

        void ODF_Preview::RenderFrame::wheelEvent (QWheelEvent*) {
          //Talk to the hand, 'cause the scroll wheel ain't listening.      
        }





        ODF_Preview::ODF_Preview (ODF* parent) :
            QWidget (&window(), Qt::Tool),
            parent (parent),
            render_frame (new RenderFrame (this))
        {
          delete render_frame->lighting;
          render_frame->lighting = parent->lighting;

          Tool::Base::VBoxLayout *main_box = new Tool::Base::VBoxLayout (this);

          main_box->addWidget (render_frame);

          QGroupBox* group_box = new QGroupBox (tr("Display settings"));
          main_box->addWidget (group_box);
          Tool::Base::GridLayout* box_layout = new Tool::Base::GridLayout;
          group_box->setLayout (box_layout);

          lock_orientation_to_image_box = new QCheckBox ("auto align");
          lock_orientation_to_image_box->setChecked (true);
          connect (lock_orientation_to_image_box, SIGNAL (stateChanged(int)), this, SLOT (lock_orientation_to_image_slot(int)));
          box_layout->addWidget (lock_orientation_to_image_box, 0, 0, 1, 2);

          interpolation_box = new QCheckBox ("interpolation");
          interpolation_box->setChecked (true);
          connect (interpolation_box, SIGNAL (stateChanged(int)), this, SLOT (interpolation_slot(int)));
          box_layout->addWidget (interpolation_box, 0, 2, 1, 2);

          show_axes_box = new QCheckBox ("show axes");
          show_axes_box->setChecked (true);
          connect (show_axes_box, SIGNAL (stateChanged(int)), this, SLOT (show_axes_slot(int)));
          box_layout->addWidget (show_axes_box, 1, 0, 1, 2);

          QLabel* label = new QLabel ("detail");
          label->setAlignment (Qt::AlignHCenter);
          box_layout->addWidget (label, 1, 2, 1, 1);
          level_of_detail_selector = new SpinBox (this);
          level_of_detail_selector->setMinimum (1);
          level_of_detail_selector->setMaximum (7);
          level_of_detail_selector->setSingleStep (1);
          level_of_detail_selector->setValue (5);
          connect (level_of_detail_selector, SIGNAL (valueChanged(int)), this, SLOT(level_of_detail_slot(int)));
          box_layout->addWidget (level_of_detail_selector, 1, 3, 1, 1);

          main_box->setStretchFactor (render_frame, 1);
          main_box->setStretchFactor (group_box, 0);

          render_frame->set_scale (parent->scale->value());
          render_frame->set_color_by_dir (parent->colour_by_direction_box->isChecked());
          render_frame->set_hide_neg_values (parent->hide_negative_values_box->isChecked());
          render_frame->set_use_lighting (parent->use_lighting_box->isChecked());
          render_frame->set_lmax (parent->lmax_selector->value());
          lock_orientation_to_image_slot (1);
          interpolation_slot (1);
          show_axes_slot (1);
          level_of_detail_slot (5);

        }




        void ODF_Preview::set (const Eigen::VectorXf& data)
        {
          render_frame->set (data);
          lock_orientation_to_image_slot (0);
        }

        void ODF_Preview::lock_orientation_to_image_slot (int)
        {
          if (lock_orientation_to_image_box->isChecked()) {
            const Projection* proj = window().get_current_mode()->get_current_projection();
            if (!proj) return;
            render_frame->reset_view();
            render_frame->set_rotation (proj->modelview());
          }
        }

        void ODF_Preview::interpolation_slot (int)
        {
          parent->update_preview();
        }

        void ODF_Preview::show_axes_slot (int)
        {
          render_frame->set_show_axes (show_axes_box->isChecked());
        }

        void ODF_Preview::level_of_detail_slot (int)
        {
          render_frame->set_LOD (level_of_detail_selector->value());
        }

        void ODF_Preview::lighting_update_slot()
        {
          // Just force a redraw - preferably something compatible with all ODF modes
          render_frame->set_show_axes (show_axes_box->isChecked());
        }




      }
    }
  }
}





