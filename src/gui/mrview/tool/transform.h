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


#ifndef __gui_mrview_tool_transform_h__
#define __gui_mrview_tool_transform_h__

#include "gui/mrview/tool/base.h"
#include "gui/mrview/mode/base.h"

namespace MR
{
  namespace GUI
  {
    namespace MRView
    {

      namespace Tool
      {


        class Transform : public Base, public Tool::CameraInteractor
        { MEMALIGN(Transform)
          Q_OBJECT
          public:
            Transform (Dock* parent);

            bool slice_move_event (const ModelViewProjection& projection, float inc) override;
            bool pan_event (const ModelViewProjection& projection) override;
            bool panthrough_event (const ModelViewProjection& projection) override;
            bool tilt_event (const ModelViewProjection& projection) override;
            bool rotate_event (const ModelViewProjection& projection) override;

          protected:
            QPushButton *activate_button;
            virtual void showEvent (QShowEvent* event) override;
            virtual void closeEvent (QCloseEvent* event) override;
            virtual void hideEvent (QHideEvent* event) override;

            void setActive (bool onoff);

          protected slots:
            void onActivate (bool);
        };

      }
    }
  }
}

#endif





