/* Copyright (c) 2008-2023 the MRtrix3 contributors.
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

