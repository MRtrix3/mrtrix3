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

            bool slice_move_event (float inc) override;
            bool pan_event () override;
            bool panthrough_event () override;
            bool tilt_event () override;
            bool rotate_event () override;

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





