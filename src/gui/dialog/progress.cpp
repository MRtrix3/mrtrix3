/*
    Copyright 2010 Brain Research Institute, Melbourne, Australia

    Written by J-Donald Tournier, 28/03/2010.

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
            QMetaObject::invokeMethod (GUI::App::application, "startProgressBar");
            p.data = new Timer;
          }
          else {
            if (reinterpret_cast<Timer*>(p.data)->elapsed() > 1.0) 
              QMetaObject::invokeMethod (GUI::App::application, "displayProgressBar", Q_ARG (QString, p.text.c_str()), Q_ARG (int, p.value), Q_ARG(bool, p.multiplier));
          }
        }


        void done (ProgressInfo& p)
        {
          INFO (MR::App::NAME + ": " + p.text + " [done]");
          if (p.data) 
            QMetaObject::invokeMethod (GUI::App::application, "doneProgressBar");
          p.data = nullptr;
        }

      }
    }
  }
}



