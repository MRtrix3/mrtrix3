/*
    Copyright 2008 Brain Research Institute, Melbourne, Australia

    Written by J-Donald Tournier, 27/06/08.

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

#include <QPushButton>
#include <QLabel>
#include <QLayout>
#include <QGroupBox>
#include <QSlider>
#include <QStyle>

#include "math/vector.h"
#include "gui/color_button.h"
#include "gui/dialog/lighting.h"


namespace MR
{
  namespace GUI
  {
    namespace Dialog
    {

      LightingSettings::LightingSettings (QWidget* parent, GL::Lighting& lighting, bool include_object_color) :
        QFrame (parent),
        info (lighting)
      {
        QColor C;
        QGridLayout* grid_layout = new QGridLayout;
        setLayout (grid_layout);
        QColorButton* cbutton;
        QSlider* slider;

        if (include_object_color) {
          C.setRgbF (info.object_color[0], info.object_color[1], info.object_color[2]);
          cbutton = new QColorButton (C);
          connect (cbutton, SIGNAL (changed (const QColor&)), this, SLOT (object_color_slot (const QColor&)));
          grid_layout->addWidget (new QLabel ("Object Colour"), 0, 0);
          grid_layout->addWidget (cbutton, 0, 1);
        }

        C.setRgbF (info.ambient_color[0], info.ambient_color[1], info.ambient_color[2]);
        cbutton = new QColorButton (C);
        connect (cbutton, SIGNAL (changed (const QColor&)), this, SLOT (ambient_color_slot (const QColor&)));
        grid_layout->addWidget (new QLabel ("Ambient Colour"), 1, 0);
        grid_layout->addWidget (cbutton, 1, 1);

        C.setRgbF (info.light_color[0], info.light_color[1], info.light_color[2]);
        cbutton = new QColorButton (C);
        connect (cbutton, SIGNAL (changed (const QColor&)), this, SLOT (diffuse_color_slot (const QColor&)));
        grid_layout->addWidget (new QLabel ("Light Colour"), 2, 0);
        grid_layout->addWidget (cbutton, 2, 1);


        slider = new QSlider (Qt::Horizontal);
        slider->setRange (0,1000);
        slider->setSliderPosition (int (info.ambient * 1000.0));
        connect (slider, SIGNAL (valueChanged (int)), this, SLOT (ambient_intensity_slot (int)));
        grid_layout->addWidget (new QLabel ("Ambient intensity"), 3, 0);
        grid_layout->addWidget (slider, 3, 1);

        slider = new QSlider (Qt::Horizontal);
        slider->setRange (0,1000);
        slider->setSliderPosition (int (info.diffuse * 1000.0));
        connect (slider, SIGNAL (valueChanged (int)), this, SLOT (diffuse_intensity_slot (int)));
        grid_layout->addWidget (new QLabel ("Diffuse intensity"), 4, 0);
        grid_layout->addWidget (slider, 4, 1);

        slider = new QSlider (Qt::Horizontal);
        slider->setRange (0,1000);
        slider->setSliderPosition (int (info.specular * 1000.0));
        connect (slider, SIGNAL (valueChanged (int)), this, SLOT (specular_intensity_slot (int)));
        grid_layout->addWidget (new QLabel ("Specular intensity"), 5, 0);
        grid_layout->addWidget (slider, 5, 1);

        slider = new QSlider (Qt::Horizontal);
        slider->setRange (10,1000);
        slider->setSliderPosition (int (info.shine * 10.0));
        connect (slider, SIGNAL (valueChanged (int)), this, SLOT (shine_slot (int)));
        grid_layout->addWidget (new QLabel ("Specular exponent"), 6, 0);
        grid_layout->addWidget (slider, 6, 1);

        elevation_slider = new QSlider (Qt::Horizontal);
        elevation_slider->setRange (0,1000);
        elevation_slider->setSliderPosition (int ( (1000.0/M_PI) *acos (info.lightpos[1]/Math::norm (info.lightpos))));
        connect (elevation_slider, SIGNAL (valueChanged (int)), this, SLOT (light_position_slot()));
        grid_layout->addWidget (new QLabel ("Light elevation"), 7, 0);
        grid_layout->addWidget (elevation_slider, 7, 1);

        azimuth_slider = new QSlider (Qt::Horizontal);
        azimuth_slider->setRange (-1000,1000);
        azimuth_slider->setSliderPosition (int ( (1000.0/M_PI) *atan2 (info.lightpos[0], info.lightpos[2])));
        connect (azimuth_slider, SIGNAL (valueChanged (int)), this, SLOT (light_position_slot()));
        grid_layout->addWidget (new QLabel ("Light azimuth"), 8, 0);
        grid_layout->addWidget (azimuth_slider, 8, 1);

        grid_layout->setColumnStretch (0,0);
        grid_layout->setColumnStretch (1,1);
        grid_layout->setColumnMinimumWidth (1, 100);
      }

      void LightingSettings::object_color_slot (const QColor& new_color)
      {
        info.object_color[0] = new_color.redF();
        info.object_color[1] = new_color.greenF();
        info.object_color[2] = new_color.blueF();
        info.update();
      }

      void LightingSettings::ambient_color_slot (const QColor& new_color)
      {
        info.ambient_color[0] = new_color.redF();
        info.ambient_color[1] = new_color.greenF();
        info.ambient_color[2] = new_color.blueF();
        info.update();
      }

      void LightingSettings::diffuse_color_slot (const QColor& new_color)
      {
        info.light_color[0] = new_color.redF();
        info.light_color[1] = new_color.greenF();
        info.light_color[2] = new_color.blueF();
        info.update();
      }


      void LightingSettings::ambient_intensity_slot (int value)
      {
        info.ambient = float (value) /1000.0;
        info.update();
      }
      void LightingSettings::diffuse_intensity_slot (int value)
      {
        info.diffuse = float (value) /1000.0;
        info.update();
      }
      void LightingSettings::specular_intensity_slot (int value)
      {
        info.specular = float (value) /1000.0;
        info.update();
      }
      void LightingSettings::shine_slot (int value)
      {
        info.shine = float (value) /10.0;
        info.update();
      }

      void LightingSettings::light_position_slot ()
      {
        float elevation = elevation_slider->value() * (M_PI/1000.0);
        float azimuth = azimuth_slider->value() * (M_PI/1000.0);
        info.lightpos[2] = sin (elevation) * cos (azimuth);
        info.lightpos[0] = sin (elevation) * sin (azimuth);
        info.lightpos[1] = -cos (elevation);
        info.update();
      }





      Lighting::Lighting (QWidget* parent, const std::string& message, GL::Lighting& lighting, bool include_object_color) :
        settings (new LightingSettings (this, lighting, include_object_color)) {
          setWindowTitle (QString (message.c_str()));
          setModal (false);
          setSizeGripEnabled (true);

          QPushButton* close_button = new QPushButton (style()->standardIcon (QStyle::SP_DialogCloseButton), tr ("&Close"));
          connect (close_button, SIGNAL (clicked()), this, SLOT (close()));

          QHBoxLayout* buttons_layout = new QHBoxLayout;
          buttons_layout->addStretch (1);
          buttons_layout->addWidget (close_button);

          QVBoxLayout* main_layout = new QVBoxLayout;
          main_layout->addWidget (settings);
          main_layout->addStretch (1);
          main_layout->addSpacing (12);
          main_layout->addLayout (buttons_layout);
          setLayout (main_layout);
        }

      

    }
  }
}
