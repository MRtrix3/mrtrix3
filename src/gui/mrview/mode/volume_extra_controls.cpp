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

        __ExtraControls::__ExtraControls (Window& main_window, Tool::Dock* parent) :
          Tool::Base (main_window, parent), 
          lighting_dialog (NULL) {
            QVBoxLayout* main_box = new QVBoxLayout (this);

            transparency_box = new QGroupBox ("Transparency");
            transparency_box->setCheckable (true);
            transparency_box->setChecked (false);
            connect (transparency_box, SIGNAL (toggled(bool)), this, SLOT (onSetTransparency()));
            QGridLayout* layout = new QGridLayout;
            main_box->addWidget (transparency_box);
            transparency_box->setLayout (layout);

            layout->addWidget (new QLabel ("transparent"), 0, 0);
            transparent_intensity = new AdjustButton (this);
            transparent_intensity->setValue (window.image() ? window.image()->intensity_min() : 0.0);
            connect (transparent_intensity, SIGNAL (valueChanged()), this, SLOT (onSetTransparency()));
            layout->addWidget (transparent_intensity, 0, 1);

            layout->addWidget (new QLabel ("opaque"), 1, 0);
            opaque_intensity = new AdjustButton (this);
            opaque_intensity->setValue (window.image() ? window.image()->intensity_max()/2.0 : 1.0);
            connect (opaque_intensity, SIGNAL (valueChanged()), this, SLOT (onSetTransparency()));
            layout->addWidget (opaque_intensity, 1, 1);

            layout->addWidget (new QLabel ("alpha"), 2, 0);
            opacity = new QSlider (Qt::Horizontal);
            opacity->setRange (0, 255);
            opacity->setValue (255);
            connect (opacity, SIGNAL (valueChanged(int)), this, SLOT (onSetTransparency()));
            layout->addWidget (opacity, 2, 1);


            lighting_box = new QGroupBox ("Lighting");
            lighting_box->setCheckable (true);
            lighting_box->setChecked (false);
            connect (lighting_box, SIGNAL (toggled(bool)), this, SLOT (onUseLighting(bool)));
            QVBoxLayout* vlayout = new QVBoxLayout;
            main_box->addWidget (lighting_box);
            lighting_box->setLayout (vlayout);

            QPushButton* button = new QPushButton ("Settings...");
            connect (button, SIGNAL (clicked()), this, SLOT (onAdvandedLighting()));
            vlayout->addWidget (button);

            main_box->addStretch ();
            setMinimumSize (main_box->minimumSize());
          }


        void __ExtraControls::showEvent (QShowEvent* event) 
        {
          connect (&window, SIGNAL (imageChanged()), this, SLOT (onScalingChanged()));
          connect (&window, SIGNAL (scalingChanged()), this, SLOT (onScalingChanged()));
          onScalingChanged();
        }


        void __ExtraControls::closeEvent (QCloseEvent* event) { window.disconnect (this); }



        void __ExtraControls::onScalingChanged () 
        {
          set_scaling_rate(); 
        }



        void __ExtraControls::onSetTransparency () 
        {
          if (transparency_box->isChecked()) {
            if (window.image()) 
              window.image()->set_transparency (transparent_intensity->value(), opaque_intensity->value(), float (opacity->value()) / 255.0);
          }
          else 
            window.image()->set_transparency ();
          window.updateGL();
        }



        void __ExtraControls::onUseLighting (bool on) 
        {
          if (window.image())
            window.image()->set_use_lighting (on);
          window.updateGL();
        }



        void __ExtraControls::onAdvandedLighting () 
        {
          if (!lighting_dialog)
            lighting_dialog = new Dialog::Lighting (this, "Advanced Lighting", window.lighting());
          lighting_dialog->show();
        }





      }
    }
  }
}




