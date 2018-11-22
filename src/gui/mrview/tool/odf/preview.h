/* Copyright (c) 2008-2019 the MRtrix3 contributors.
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

#ifndef __gui_mrview_tool_odf_preview_h__
#define __gui_mrview_tool_odf_preview_h__

#include "gui/dwi/render_frame.h"

#include "gui/mrview/tool/odf/odf.h"
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

        class ODF_Preview : public QWidget
        { MEMALIGN(ODF_Preview)
            Q_OBJECT

            class RenderFrame : public DWI::RenderFrame 
            { MEMALIGN(RenderFrame)
              public:
                RenderFrame (QWidget* parent);

                void set_colour (const QColor& c) {
                  renderer.set_colour (c);
                }
                
              protected:
                Window& window () const { return *Window::main; }
                virtual void wheelEvent (QWheelEvent*);
            };

          public:
            ODF_Preview (ODF*);
            void set (const Eigen::VectorXf&);
            bool interpolate() const { return interpolation_box->isChecked(); }
            void set_lod_enabled (const bool i) { level_of_detail_selector->setEnabled (i); }
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





