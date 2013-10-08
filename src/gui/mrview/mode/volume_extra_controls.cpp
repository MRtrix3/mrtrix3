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

#include "gui/mrview/mode/volume_extra_controls.h"

namespace MR
{
  namespace GUI
  {
    namespace MRView
    {
      namespace Mode
      {

        VolumeExtraControls::VolumeExtraControls (Window& main_window, Tool::Dock* parent) :
          Tool::Base (main_window, parent) {
            QVBoxLayout* main_box = new QVBoxLayout (this);

            QGroupBox* transparency_box = new QGroupBox ("Transparency");
            QGridLayout* layout = new QGridLayout;
            main_box->addWidget (transparency_box);
            transparency_box->setLayout (layout);

            layout->addWidget (new QLabel ("transparent"), 0, 0);
            transparent_intensity = new AdjustButton (this);
            connect (transparent_intensity, SIGNAL (valueChanged()), this, SLOT (onSetTransparency()));
            layout->addWidget (transparent_intensity, 0, 1);

            layout->addWidget (new QLabel ("opaque"), 1, 0);
            opaque_intensity = new AdjustButton (this);
            connect (opaque_intensity, SIGNAL (valueChanged()), this, SLOT (onSetTransparency()));
            layout->addWidget (opaque_intensity, 1, 1);

            layout->addWidget (new QLabel ("alpha"), 2, 0);
            opacity = new QSlider (Qt::Horizontal);
            opacity->setRange (0, 255);
            opacity->setValue (255);
            connect (opacity, SIGNAL (valueChanged(int)), this, SLOT (onSetTransparency()));
            layout->addWidget (opacity, 2, 1);


            lighting_box = new QCheckBox ("Lighting", this);
            lighting_box->setChecked (true);
            connect (lighting_box, SIGNAL (toggled(bool)), this, SLOT (onUseLighting(bool)));
            main_box->addWidget (lighting_box);

            main_box->addStretch ();
            setMinimumSize (main_box->minimumSize());
          }


        void VolumeExtraControls::showEvent (QShowEvent* event) 
        {
          connect (&window, SIGNAL (imageChanged()), this, SLOT (onImageChanged()));
          connect (&window, SIGNAL (scalingChanged()), this, SLOT (onScalingChanged()));
          onImageChanged();
        }


        void VolumeExtraControls::closeEvent (QCloseEvent* event) { window.disconnect (this); }

        void VolumeExtraControls::onScalingChanged ()
        {
          if (!finite (transparent_intensity->value()) || 
              !finite (opaque_intensity->value())) 
            onImageChanged();
        }


        void VolumeExtraControls::onImageChanged () 
        {
          if (!window.image()) {
            transparent_intensity->clear();
            opaque_intensity->clear();
            lighting_box->setChecked (false);
            setEnabled (false);
            return;
          }

          if (!finite (window.image()->transparent_intensity) ||
              !finite (window.image()->opaque_intensity) ||
              !finite (window.image()->alpha) ) {
            if (finite (window.image()->intensity_min()) && 
                finite (window.image()->intensity_max())) {
              window.image()->transparent_intensity = window.image()->intensity_min();
              window.image()->opaque_intensity = window.image()->intensity_max();
              window.image()->alpha = opacity->value() / 255.0;
            }
            else {
              transparent_intensity->clear();
              opaque_intensity->clear();
            }
          }

          if (finite (window.image()->transparent_intensity) &&
              finite (window.image()->opaque_intensity) &&
              finite (window.image()->alpha) ) {
            transparent_intensity->setValue (window.image()->transparent_intensity);
            opaque_intensity->setValue (window.image()->opaque_intensity);
            opacity->setValue (window.image()->alpha * 255.0);
            float rate = window.image() ? window.image()->scaling_rate() : 0.0;
            transparent_intensity->setRate (rate);
            opaque_intensity->setRate (rate);
            onSetTransparency();
          }

          lighting_box->setChecked (window.image()->lighting_enabled());
          setEnabled (true);
        }




        void VolumeExtraControls::onSetTransparency () 
        {
          if (window.image()) {
            window.image()->set_transparency (
                transparent_intensity->value(), 
                opaque_intensity->value(),
                float (opacity->value()) / 255.0);
            window.updateGL();
          }
        }



        void VolumeExtraControls::onUseLighting (bool on) 
        {
          if (window.image())
            window.image()->set_use_lighting (on);
          window.updateGL();
        }




      }
    }
  }
}




