/*
 * Copyright (c) 2008-2018 the MRtrix3 contributors.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, you can obtain one at http://mozilla.org/MPL/2.0/
 *
 * MRtrix3 is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * For more details, see http://www.mrtrix.org/
 */


#ifndef __gui_mrview_mode_lightbox_gui_h__
#define __gui_mrview_mode_lightbox_gui_h__

#include "gui/mrview/spin_box.h"

namespace MR
{
  namespace GUI
  {
    namespace MRView
    {
      namespace Mode
      {
        namespace LightBoxViewControls
        {
          class LightBoxEditButton : public SpinBox
          { NOMEMALIGN
            public:
              LightBoxEditButton(QWidget* parent, const QString &suffix)
                : LightBoxEditButton(parent, 1, 100, 1, suffix) {}

              LightBoxEditButton(QWidget* parent,
                  int min = 1, int max = 100, int change_rate = 1,
                  const QString& suffix = nullptr) :
                SpinBox(parent) {
                setMinimum(min);
                setMaximum(max);
                setSingleStep(change_rate);
                setSuffix(suffix);
                setMaximumWidth(80);
              }
          };

        }
      }
    }
  }
}


#endif
