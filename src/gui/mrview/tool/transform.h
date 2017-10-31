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


        class Transform : public Base, public Mode::ModeGuiVisitor
        { MEMALIGN(Transform)
          Q_OBJECT
          public:
            Transform (Dock* parent);

          private slots:
            void onImageChanged ();
            void onImageVisibilityChanged (bool);

          private:
            QPushButton *activated;
        };

      }
    }
  }
}

#endif





