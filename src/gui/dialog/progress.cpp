/*
 * Copyright (c) 2008-2016 the MRtrix3 contributors
 * 
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/
 * 
 * MRtrix is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * 
 * For more details, see www.mrtrix.org
 * 
 */

#include <cassert>
#include "gui/gui.h"
#include "gui/dialog/progress.h"

namespace MR
{
  namespace GUI
  {
    namespace Dialog
    {
      namespace ProgressBar
      {

        void display (ProgressInfo& p)
        {
          if (!p.data) {
            INFO (MR::App::NAME + ": " + p.text);
            QMetaObject::invokeMethod (GUI::App::application, "startProgressBar", Qt::DirectConnection);
            p.data = new Timer;
          }
          else {
            if (reinterpret_cast<Timer*>(p.data)->elapsed() > 1.0)
              QMetaObject::invokeMethod (GUI::App::application, "displayProgressBar", Qt::DirectConnection,
                  Q_ARG (QString, (p.text + p.ellipsis).c_str()), Q_ARG (int, p.value), Q_ARG(bool, p.multiplier));
          }
        }


        void done (ProgressInfo& p)
        {
          INFO (MR::App::NAME + ": " + p.text + " [done]");
          if (p.data) 
            QMetaObject::invokeMethod (GUI::App::application, "doneProgressBar", Qt::DirectConnection);
          p.data = nullptr;
        }

      }
    }
  }
}



