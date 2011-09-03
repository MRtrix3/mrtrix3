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

#ifndef __gui_dialog_lighting_h__
#define __gui_dialog_lighting_h__

#include <QDialog>

#include "mrtrix.h"
#include "gui/opengl/lighting.h"

class QSlider;

namespace MR
{
  namespace GUI
  {
    namespace Dialog
    {

      class Lighting : public QDialog
      {
          Q_OBJECT

        public:
          Lighting (QWidget* parent, const std::string& message, GL::Lighting& lighting);
          ~Lighting () { }

        protected:
          GL::Lighting&  info;
          QSlider* elevation_slider, *azimuth_slider;

        protected slots:
          void object_color_slot (const QColor& new_color);
          void ambient_color_slot (const QColor& new_color);
          void diffuse_color_slot (const QColor& new_color);
          void ambient_intensity_slot (int value);
          void diffuse_intensity_slot (int value);
          void specular_intensity_slot (int value);
          void shine_slot (int value);
          void light_position_slot ();
      };

    }
  }
}

#endif

