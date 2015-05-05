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

#ifndef __gui_mrview_tool_odf_preview_h__
#define __gui_mrview_tool_odf_preview_h__

#include "gui/mrview/tool/base.h"
#include "gui/mrview/window.h"

namespace MR
{
  namespace GUI
  {
    namespace DWI {
      class Renderer;
      class RenderFrame;
    }
    namespace Dialog {
      class Lighting;
    }
    namespace GL {
      class Lighting;
    }

    namespace MRView
    {
      namespace Tool
      {

        class ODF;

        class ODF_Preview : public Base
        {
            Q_OBJECT
          public:
            ODF_Preview (Window&, Dock*, ODF*);
            void set (const Math::Vector<float>&);
            bool interpolate() const { return interpolation_box->isChecked(); }
            size_t lmax() const { return lmax_selector->value(); }
          private slots:
            void lock_orientation_to_image_slot (int);
            void hide_negative_lobes_slot (int);
            void colour_by_direction_slot (int);
            void interpolation_slot (int);
            void show_axes_slot (int);
            void lmax_slot (int);
            void level_of_detail_slot (int);
            void use_lighting_slot (int);
            void lighting_settings_slot (bool);
            void hide_event ();
          protected:
            ODF* parent;
            class RenderFrame;
            RenderFrame* render_frame;
            QCheckBox *lock_orientation_to_image_box, *hide_negative_lobes_box;
            QCheckBox *colour_by_direction_box, *interpolation_box, *show_axes_box;
            QSpinBox *lmax_selector, *level_of_detail_selector;
            QCheckBox *use_lighting_box;
            Dialog::Lighting *lighting_dialog;
        };

      }
    }
  }
}

#endif





