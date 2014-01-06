/*
   Copyright 2009 Brain Research Institute, Melbourne, Australia

   Written by J-Donald Tournier, 13/11/09.

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

#include <QLabel>

#include "mrtrix.h"
#include "gui/mrview/window.h"
#include "gui/mrview/mode/base.h"
#include "gui/mrview/mode/volume.h"
#include "gui/mrview/tool/view.h"
#include "gui/mrview/adjust_button.h"

#define FOV_RATE_MULTIPLIER 0.01

namespace MR
{
  namespace GUI
  {
    namespace MRView
    {
      namespace Tool
      {


        View::View (Window& main_window, Dock* parent) : 
          Base (main_window, parent)
        {
          VBoxLayout* main_box = new VBoxLayout (this);

          QGroupBox* group_box = new QGroupBox ("FOV");
          main_box->addWidget (group_box);
          HBoxLayout* hlayout = new HBoxLayout;
          group_box->setLayout (hlayout);

          fov = new AdjustButton (this);
          connect (fov, SIGNAL (valueChanged()), this, SLOT (onSetFOV()));
          hlayout->addWidget (fov);

          plane_combobox = new QComboBox;
          plane_combobox->insertItem (0, "Sagittal");
          plane_combobox->insertItem (1, "Coronal");
          plane_combobox->insertItem (2, "Axial");
          connect (plane_combobox, SIGNAL (activated(int)), this, SLOT (onSetPlane(int)));
          hlayout->addWidget (plane_combobox);

          group_box = new QGroupBox ("Focus");
          main_box->addWidget (group_box);
          hlayout = new HBoxLayout;
          group_box->setLayout (hlayout);

          focus_x = new AdjustButton (this);
          connect (focus_x, SIGNAL (valueChanged()), this, SLOT (onSetFocus()));
          hlayout->addWidget (focus_x);

          focus_y = new AdjustButton (this);
          connect (focus_y, SIGNAL (valueChanged()), this, SLOT (onSetFocus()));
          hlayout->addWidget (focus_y);

          focus_z = new AdjustButton (this);
          connect (focus_z, SIGNAL (valueChanged()), this, SLOT (onSetFocus()));
          hlayout->addWidget (focus_z);

          group_box = new QGroupBox ("Intensity scaling");
          main_box->addWidget (group_box);
          hlayout = new HBoxLayout;
          group_box->setLayout (hlayout);

          min_entry = new AdjustButton (this);
          connect (min_entry, SIGNAL (valueChanged()), this, SLOT (onSetScaling()));
          hlayout->addWidget (min_entry);

          max_entry = new AdjustButton (this);
          connect (max_entry, SIGNAL (valueChanged()), this, SLOT (onSetScaling()));
          hlayout->addWidget (max_entry);


          GridLayout* layout;
          layout = new GridLayout;
          main_box->addLayout (layout);



          transparency_box = new QGroupBox ("Transparency");
          main_box->addWidget (transparency_box);
          VBoxLayout* vlayout = new VBoxLayout;
          transparency_box->setLayout (vlayout);

          hlayout = new HBoxLayout;
          vlayout->addLayout (hlayout);

          transparent_intensity = new AdjustButton (this);
          connect (transparent_intensity, SIGNAL (valueChanged()), this, SLOT (onSetTransparency()));
          hlayout->addWidget (transparent_intensity, 0, 0);

          opaque_intensity = new AdjustButton (this);
          connect (opaque_intensity, SIGNAL (valueChanged()), this, SLOT (onSetTransparency()));
          hlayout->addWidget (opaque_intensity);

          hlayout = new HBoxLayout;
          vlayout->addLayout (hlayout);

          hlayout->addWidget (new QLabel ("alpha"));
          opacity = new QSlider (Qt::Horizontal);
          opacity->setRange (0, 255);
          opacity->setValue (255);
          connect (opacity, SIGNAL (valueChanged(int)), this, SLOT (onSetTransparency()));
          hlayout->addWidget (opacity);


          threshold_box = new QGroupBox ("Thresholds");
          main_box->addWidget (threshold_box);
          hlayout = new HBoxLayout;
          threshold_box->setLayout (hlayout);

          lower_threshold_check_box = new QCheckBox (this);
          hlayout->addWidget (lower_threshold_check_box);
          lower_threshold = new AdjustButton (this);
          lower_threshold->setValue (window.image() ? window.image()->intensity_min() : 0.0);
          connect (lower_threshold_check_box, SIGNAL (clicked(bool)), this, SLOT (onCheckThreshold(bool)));
          connect (lower_threshold, SIGNAL (valueChanged()), this, SLOT (onSetTransparency()));
          hlayout->addWidget (lower_threshold);

          upper_threshold_check_box = new QCheckBox (this);
          hlayout->addWidget (upper_threshold_check_box);
          upper_threshold = new AdjustButton (this);
          upper_threshold->setValue (window.image() ? window.image()->intensity_max() : 1.0);
          connect (upper_threshold_check_box, SIGNAL (clicked(bool)), this, SLOT (onCheckThreshold(bool)));
          connect (upper_threshold, SIGNAL (valueChanged()), this, SLOT (onSetTransparency()));
          hlayout->addWidget (upper_threshold);


          clip_box = new QGroupBox ("Clip planes");
          layout = new GridLayout;
          layout->setSpacing (0);
          main_box->addWidget (clip_box);
          clip_box->setLayout (layout);
          for (size_t n = 0; n < 3; ++n) {
            clip_on_button[n] = new QPushButton (n == 0 ? "axial" : ( n == 1 ? "sagittal" : "coronal" ), this);
            clip_on_button[n]->setCheckable (true);
            connect (clip_on_button[n], SIGNAL (toggled(bool)), &window, SLOT(updateGL()));
            layout->addWidget (clip_on_button[n], 0, n);

            QPushButton* clip_invert_button = new QPushButton ("invert", this);
            if (n == 0) connect (clip_invert_button, SIGNAL (clicked(bool)), this, SLOT(onClip0Invert()));
            else if (n == 1) connect (clip_invert_button, SIGNAL (clicked(bool)), this, SLOT(onClip1Invert()));
            else connect (clip_invert_button, SIGNAL (clicked(bool)), this, SLOT(onClip2Invert()));
            layout->addWidget (clip_invert_button, 1, n);

            clip_edit_button[n] = new QPushButton ("modify", this);
            clip_edit_button[n]->setToolTip ("when checked, standard image manipulation actions apply to clip plane instead");
            layout->addWidget (clip_edit_button[n], 2, n);
            clip_edit_button[n]->setCheckable (true);
          }
          clip_modify_button = new QPushButton ("toggle modify all", this);
          connect (clip_modify_button, SIGNAL (clicked(bool)), this, SLOT (onClipModify()));
          layout->addWidget (clip_modify_button, 3, 0, 1, 3);
          clip_modify_button = new QPushButton ("reset", this);
          connect (clip_modify_button, SIGNAL (clicked(bool)), this, SLOT (onClipReset()));
          layout->addWidget (clip_modify_button, 4, 0, 1, 3);

          main_box->addStretch ();
        }





        void View::showEvent (QShowEvent* event) 
        {
          connect (&window, SIGNAL (imageChanged()), this, SLOT (onImageChanged()));
          connect (&window, SIGNAL (focusChanged()), this, SLOT (onFocusChanged()));
          connect (&window, SIGNAL (planeChanged()), this, SLOT (onPlaneChanged()));
          connect (&window, SIGNAL (scalingChanged()), this, SLOT (onScalingChanged()));
          connect (&window, SIGNAL (modeChanged()), this, SLOT (onModeChanged()));
          connect (&window, SIGNAL (fieldOfViewChanged()), this, SLOT (onFOVChanged()));
          onPlaneChanged();
          onFocusChanged();
          onScalingChanged();
          onModeChanged();
          onImageChanged();
          onFOVChanged();
        }





        void View::closeEvent (QCloseEvent* event) 
        {
          window.disconnect (this);
        }



        void View::onImageChanged () 
        {
          onScalingChanged();

          float rate = window.image()->focus_rate();
          focus_x->setRate (rate);
          focus_y->setRate (rate);
          focus_z->setRate (rate);

          set_transparency_from_image();

          lower_threshold_check_box->setChecked (window.image()->use_discard_lower());
          upper_threshold_check_box->setChecked (window.image()->use_discard_upper());
        }





        void View::onFocusChanged () 
        {
          focus_x->setValue (window.focus()[0]);
          focus_y->setValue (window.focus()[1]);
          focus_z->setValue (window.focus()[2]);
        }



        void View::onFOVChanged () 
        {
          fov->setValue (window.FOV());
          fov->setRate (FOV_RATE_MULTIPLIER * fov->value());
        }





        void View::onSetFocus () 
        {
          try {
            window.set_focus (Point<> (focus_x->value(), focus_y->value(), focus_z->value()));
            window.updateGL();
          }
          catch (Exception) { }
        }





        void View::onModeChanged () 
        {
          transparency_box->setEnabled (window.get_current_mode()->features & Mode::ShaderTransparency);
          threshold_box->setEnabled (window.get_current_mode()->features & Mode::ShaderTransparency);
          clip_box->setEnabled (window.get_current_mode()->features & Mode::ShaderClipping);
        }





        void View::onClipModify () 
        {
          bool any_on = clip_edit_button[0]->isChecked() || clip_edit_button[1]->isChecked() || clip_edit_button[2]->isChecked();

          clip_edit_button[0]->setChecked (!any_on && clip_on_button[0]->isChecked());
          clip_edit_button[1]->setChecked (!any_on && clip_on_button[1]->isChecked());
          clip_edit_button[2]->setChecked (!any_on && clip_on_button[2]->isChecked());
        }

        void View::onClipReset () 
        {
          static_cast<Mode::Volume*> (window.get_current_mode())->reset_clip_planes();
          window.updateGL();
        }


        void View::onClip0Invert () 
        {
          static_cast<Mode::Volume*> (window.get_current_mode())->invert_clip_plane (0);
        }

        void View::onClip1Invert () 
        {
          static_cast<Mode::Volume*> (window.get_current_mode())->invert_clip_plane (1);
        }

        void View::onClip2Invert () 
        {
          static_cast<Mode::Volume*> (window.get_current_mode())->invert_clip_plane (2);
        }



        void View::onSetTransparency () 
        {
          assert (window.image()); 
          window.image()->transparent_intensity = transparent_intensity->value();
          window.image()->opaque_intensity = opaque_intensity->value();
          window.image()->alpha = float (opacity->value()) / 255.0;
          window.image()->lessthan = lower_threshold->value(); 
          window.image()->greaterthan = upper_threshold->value(); 
          window.updateGL();
        }





        void View::onPlaneChanged () 
        {
          plane_combobox->setCurrentIndex (window.plane());
        }





        void View::onSetPlane (int index) 
        {
          window.set_plane (index);
          window.updateGL();
        }




        void View::onCheckThreshold (bool) 
        {
          assert (window.image());
          assert (threshold_box->isEnabled());
          window.image()->set_use_discard_lower (lower_threshold_check_box->isChecked());
          window.image()->set_use_discard_upper (upper_threshold_check_box->isChecked());
          window.updateGL();
        }







        void View::set_transparency_from_image () 
        {
          if (!finite (window.image()->transparent_intensity) ||
              !finite (window.image()->opaque_intensity) ||
              !finite (window.image()->alpha) ||
              !finite (window.image()->lessthan) ||
              !finite (window.image()->greaterthan)) { // reset:
            if (!finite (window.image()->intensity_min()) || 
                !finite (window.image()->intensity_max()))
              return;

            if (!finite (window.image()->transparent_intensity))
              window.image()->transparent_intensity = window.image()->intensity_min();
            if (!finite (window.image()->opaque_intensity))
              window.image()->opaque_intensity = window.image()->intensity_max();
            if (!finite (window.image()->alpha))
              window.image()->alpha = opacity->value() / 255.0;
            if (!finite (window.image()->lessthan))
              window.image()->lessthan = window.image()->intensity_min();
            if (!finite (window.image()->greaterthan))
              window.image()->greaterthan = window.image()->intensity_max();
          }

          assert (finite (window.image()->transparent_intensity));
          assert (finite (window.image()->opaque_intensity));
          assert (finite (window.image()->alpha));
          assert (finite (window.image()->lessthan));
          assert (finite (window.image()->greaterthan));

          transparent_intensity->setValue (window.image()->transparent_intensity);
          opaque_intensity->setValue (window.image()->opaque_intensity);
          opacity->setValue (window.image()->alpha * 255.0);
          lower_threshold->setValue (window.image()->lessthan);
          upper_threshold->setValue (window.image()->greaterthan);
          lower_threshold_check_box->setChecked (window.image()->use_discard_lower());
          upper_threshold_check_box->setChecked (window.image()->use_discard_upper());

          float rate = window.image() ? window.image()->scaling_rate() : 0.0;
          transparent_intensity->setRate (rate);
          opaque_intensity->setRate (rate);
          lower_threshold->setRate (rate);
          upper_threshold->setRate (rate);
        }




        void View::onSetScaling ()
        {
          if (window.image()) {
            window.image()->set_windowing (min_entry->value(), max_entry->value());
            window.updateGL();
          }
        }




        void View::onSetFOV ()
        {
          if (window.image()) {
            window.set_FOV (fov->value());
            fov->setRate (FOV_RATE_MULTIPLIER * fov->value());
            window.updateGL();
          }
        }



        void View::onScalingChanged ()
        {
          if (window.image()) {
            min_entry->setValue (window.image()->scaling_min());
            max_entry->setValue (window.image()->scaling_max());
            float rate = window.image()->scaling_rate();
            min_entry->setRate (rate);
            max_entry->setRate (rate);
          }
        }

      }
    }
  }
}






