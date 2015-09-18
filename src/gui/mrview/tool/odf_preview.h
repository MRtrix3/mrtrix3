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

#include "gui/dwi/render_frame.h"

#include "gui/mrview/tool/odf.h"
#include "gui/mrview/spin_box.h"
#include "gui/mrview/window.h"

namespace MR
{
  namespace GUI
  {
    namespace MRView
    {
      namespace Tool
      {

        class ODF::Preview : public QWidget
        {
            Q_OBJECT

            class RenderFrame : public DWI::RenderFrame
            {
              public:
                RenderFrame (QWidget* parent);
                
                void set_scale (float sc) {
                  scale = sc;
                  update();
                }
                
              protected:
                Window& window () const { return *Window::main; }
                virtual void wheelEvent (QWheelEvent*);
            };

          public:
            Preview (ODF*);
            void set (const Math::Vector<float>&);
            bool interpolate() const { return interpolation_box->isChecked(); }
            Window& window () const { return *Window::main; }
          private slots:
            void lock_orientation_to_image_slot (int);
            void interpolation_slot (int);
            void show_axes_slot (int);
            void level_of_detail_slot (int);
            void lighting_update_slot();
          protected:
            ODF* parent;
            RenderFrame* render_frame;
            QCheckBox *lock_orientation_to_image_box;
            QCheckBox *interpolation_box, *show_axes_box;
            SpinBox *level_of_detail_selector;
            friend class ODF;
        };

      }
    }
  }
}

#endif





