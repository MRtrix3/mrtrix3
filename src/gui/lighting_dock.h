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


#ifndef __gui_lighting_dock_h__
#define __gui_lighting_dock_h__

#include "gui/opengl/lighting.h"

namespace MR
{
  namespace GUI
  {
    class LightingSettings : public QFrame
    { NOMEMALIGN
      Q_OBJECT

      public:
        LightingSettings (QWidget* parent, GL::Lighting& lighting);
        ~LightingSettings () { }

      protected:
        GL::Lighting&  info;
        QSlider* elevation_slider, *azimuth_slider;

      protected slots:
        void ambient_intensity_slot (int value);
        void diffuse_intensity_slot (int value);
        void specular_intensity_slot (int value);
        void shine_slot (int value);
        void light_position_slot ();
    };

    class LightingDock : public QDockWidget
    { NOMEMALIGN
      public:
        LightingDock (const std::string& title, GL::Lighting& lighting);
      private:
        LightingSettings* settings;
    };
  }
}

#endif

