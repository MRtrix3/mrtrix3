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
#if QT_VERSION >= 0x050400
          QOpenGLContext* context = QOpenGLContext::currentContext();
          QSurface* surface = context->surface();
#endif
          if (!p.data) {
            INFO (App::NAME + ": " + p.text);
            p.data = new QProgressDialog (p.text.c_str(), "Cancel", 0, p.multiplier ? 100 : 0);
            reinterpret_cast<QProgressDialog*> (p.data)->setWindowModality (Qt::WindowModal);
          }
          reinterpret_cast<QProgressDialog*> (p.data)->setValue (p.value);

#if QT_VERSION >= 0x050400
          context->makeCurrent (surface);
#endif
        }


        void done (ProgressInfo& p)
        {
#if QT_VERSION >= 0x050400
          QOpenGLContext* context = QOpenGLContext::currentContext();
          QSurface* surface = context->surface();
#endif
         
          INFO (App::NAME + ": " + p.text + " [done]");
          delete reinterpret_cast<QProgressDialog*> (p.data);
          p.data = nullptr;

#if QT_VERSION >= 0x050400
          context->makeCurrent (surface);
#endif
        }

      }
    }
  }
}



