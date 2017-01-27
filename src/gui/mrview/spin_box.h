/* Copyright (c) 2008-2017 the MRtrix3 contributors
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

#ifndef __gui_spin_box_h__
#define __gui_spin_box_h__

#include "mrtrix.h"
#include "gui/opengl/gl.h"

namespace MR
{
  namespace GUI
  {

    class SpinBox : public QSpinBox
    { NOMEMALIGN
        Q_OBJECT

      public:
        using QSpinBox::QSpinBox;

      private:
        void timerEvent (QTimerEvent* event)
        {
          // Process all events, which may include a mouse release event
          // Only allow the timer to trigger additional value changes if the user
          //   has in fact held the mouse button, rather than the timer expiry
          //   simply appearing before the mouse release in the event queue
          qApp->processEvents();
          if (QApplication::mouseButtons() & Qt::LeftButton)
            QSpinBox::timerEvent (event);
        }

    };


  }
}

#endif

