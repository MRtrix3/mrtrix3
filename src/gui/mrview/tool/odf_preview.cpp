/*
   Copyright 2009 Brain Research Institute, Melbourne, Australia

   Written by J-Donald Tournier and Robert E. Smith, 2015.

   This file is part of MRtrix.

   MRtrix is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   MRtrix is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with MRtrix.  If not, see <http://www.gnu.org/licenses/>.

*/

#include "gui/mrview/tool/odf_preview.h"
#include "gui/dialog/lighting.h"
#include "gui/dwi/render_frame.h"
#include "gui/mrview/window.h"
#include "gui/mrview/tool/odf.h"
#include "gui/mrview/mode/base.h"

namespace MR
{
  namespace GUI
  {
    namespace MRView
    {
      namespace Tool
      {




        class ODF_Preview::RenderFrame : public DWI::RenderFrame
        {
          public:
            RenderFrame (QWidget* parent, Window& window) :
              DWI::RenderFrame (parent),
              window (window) { }
          protected:
            Window& window;

            virtual void resizeGL (int w, int h) {
              makeCurrent();
              DWI::RenderFrame::resizeGL (w,h);
              window.makeGLcurrent();
            }

            virtual void initializeGL () {
              makeCurrent();
              DWI::RenderFrame::initializeGL();
              window.makeGLcurrent();
            }
            virtual void paintGL () {
              makeCurrent();
              DWI::RenderFrame::paintGL();
              window.makeGLcurrent();
            }
        };





        ODF_Preview::ODF_Preview (Window& main_window, ODF* parent) :
            QDialog (parent),
            window (main_window),
            parent (parent),
            render_frame (new RenderFrame (this, main_window)),
            lighting_dialog (nullptr)
        {
          setWindowTitle ("ODF preview pane");
          setModal (false);
          setSizeGripEnabled (true);
          setWindowFlags(windowFlags() | Qt::WindowStaysOnTopHint);

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
          interpolation_box->setChecked (false);
          connect (interpolation_box, SIGNAL (stateChanged(int)), this, SLOT (interpolation_slot(int)));
          box_layout->addWidget (interpolation_box, 0, 2, 1, 2);

          show_axes_box = new QCheckBox ("show axes");
          show_axes_box->setChecked (true);
          connect (show_axes_box, SIGNAL (stateChanged(int)), this, SLOT (show_axes_slot(int)));
          box_layout->addWidget (show_axes_box, 1, 0, 1, 2);

          colour_by_direction_box = new QCheckBox ("colour by direction");
          colour_by_direction_box->setChecked (true);
          connect (colour_by_direction_box, SIGNAL (stateChanged(int)), this, SLOT (colour_by_direction_slot(int)));
          box_layout->addWidget (colour_by_direction_box, 1, 2, 1, 2);

          hide_negative_lobes_box = new QCheckBox ("hide negative lobes");
          hide_negative_lobes_box->setChecked (true);
          connect (hide_negative_lobes_box, SIGNAL (stateChanged(int)), this, SLOT (hide_negative_lobes_slot(int)));
          box_layout->addWidget (hide_negative_lobes_box, 2, 2, 1, 2);

          QLabel* label = new QLabel ("lmax");
          label->setAlignment (Qt::AlignHCenter);
          box_layout->addWidget (label, 3, 0);
          lmax_selector = new QSpinBox (this);
          lmax_selector->setMinimum (2);
          lmax_selector->setMaximum (16);
          lmax_selector->setSingleStep (2);
          lmax_selector->setValue (8);
          connect (lmax_selector, SIGNAL (valueChanged(int)), this, SLOT(lmax_slot(int)));
          box_layout->addWidget (lmax_selector, 3, 1);

          label = new QLabel ("detail");
          label->setAlignment (Qt::AlignHCenter);
          box_layout->addWidget (label, 3, 2);
          level_of_detail_selector = new QSpinBox (this);
          level_of_detail_selector->setMinimum (1);
          level_of_detail_selector->setMaximum (7);
          level_of_detail_selector->setSingleStep (1);
          level_of_detail_selector->setValue (4);
          connect (level_of_detail_selector, SIGNAL (valueChanged(int)), this, SLOT(level_of_detail_slot(int)));
          box_layout->addWidget (level_of_detail_selector, 3, 3);

          use_lighting_box = new QCheckBox ("use lighting");
          use_lighting_box->setCheckable (true);
          use_lighting_box->setChecked (true);
          connect (use_lighting_box, SIGNAL (stateChanged(int)), this, SLOT (use_lighting_slot(int)));
          box_layout->addWidget (use_lighting_box, 2, 0, 1, 2);

          QPushButton *lighting_settings_button = new QPushButton ("lighting...", this);
          connect (lighting_settings_button, SIGNAL(clicked(bool)), this, SLOT (lighting_settings_slot (bool)));
          box_layout->addWidget (lighting_settings_button, 5, 0, 1, 4);

          main_box->setStretchFactor (render_frame, 1);
          main_box->setStretchFactor (group_box, 0);

          hide_negative_lobes_slot (0);
          show_axes_slot (0);
          colour_by_direction_slot (0);
          use_lighting_slot (0);
          lmax_slot (0);
          level_of_detail_slot (0);
          lock_orientation_to_image_slot (0);
        }




        void ODF_Preview::set (const Math::Vector<float>& data)
        {
          render_frame->set (data);
          lock_orientation_to_image_slot (0);
        }

        void ODF_Preview::hide()
        {
          parent->show_preview_button->setChecked (false);
          if (lighting_dialog)
            lighting_dialog->hide();
          QDialog::hide();
        }

        void ODF_Preview::lock_orientation_to_image_slot (int)
        {
          if (lock_orientation_to_image_box->isChecked()) {
            const Projection* proj = window.get_current_mode()->get_current_projection();
            if (!proj) return;
            render_frame->set_rotation (proj->modelview());
          }
        }

        void ODF_Preview::hide_negative_lobes_slot (int)
        {
          render_frame->set_hide_neg_lobes (hide_negative_lobes_box->isChecked());
        }

        void ODF_Preview::colour_by_direction_slot (int)
        {
          render_frame->set_color_by_dir (colour_by_direction_box->isChecked());
        }

        void ODF_Preview::interpolation_slot (int)
        {
          parent->update_preview();
        }

        void ODF_Preview::show_axes_slot (int)
        {
          render_frame->set_show_axes (show_axes_box->isChecked());
        }

        void ODF_Preview::lmax_slot (int)
        {
          render_frame->set_lmax (lmax_selector->value());
        }

        void ODF_Preview::level_of_detail_slot (int)
        {
          render_frame->set_LOD (level_of_detail_selector->value());
        }

        void ODF_Preview::use_lighting_slot (int)
        {
          render_frame->set_use_lighting (use_lighting_box->isChecked());
        }

        void ODF_Preview::lighting_settings_slot (bool)
        {
          assert (render_frame);
          if (!lighting_dialog)
            lighting_dialog = new Dialog::Lighting (&window, "Lighting for ODF preview", *render_frame->lighting);
          lighting_dialog->show();
        }

        void ODF_Preview::closeEvent (QCloseEvent*)
        {
          hide();
        }





















      }
    }
  }
}





