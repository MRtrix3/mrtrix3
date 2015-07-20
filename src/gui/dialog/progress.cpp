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

        namespace {
          QWidget* main_window = nullptr;
          struct Data {
            Data () : dialog (nullptr) { }
            QProgressDialog* dialog;
            Timer timer;
          };
        }

        void set_main_window (QWidget* window) {
          main_window = window;
        }

        void display (ProgressInfo& p)
        {
          if (!p.data) {
            INFO (App::NAME + ": " + p.text);
            main_window->setUpdatesEnabled (false);
            p.data = new Data;
          }
          else {
#if QT_VERSION >= 0x050400
            QOpenGLContext* context = QOpenGLContext::currentContext();
            QSurface* surface = context ? context->surface() : nullptr;
#endif
            Data* data = reinterpret_cast<Data*> (p.data);
            if (!data->dialog) {
              if (data->timer.elapsed() > 1.0) {
                data->dialog = new QProgressDialog (p.text.c_str(), "Cancel", 0, p.multiplier ? 100 : 0);
                data->dialog->setWindowModality (Qt::ApplicationModal);
                data->dialog->show();
                qApp->processEvents();
              }
            }
            else {
              data->dialog->setValue (p.value);
              qApp->processEvents();
            }

#if QT_VERSION >= 0x050400
            if (context) {
              assert (surface);
              context->makeCurrent (surface);
            }
#endif
          }
        }


        void done (ProgressInfo& p)
        {
          INFO (App::NAME + ": " + p.text + " [done]");
          if (p.data) {
            Data* data = reinterpret_cast<Data*> (p.data);
            if (data->dialog) {
#if QT_VERSION >= 0x050400
              QOpenGLContext* context = QOpenGLContext::currentContext();
              QSurface* surface = context ? context->surface() : nullptr;
#endif
              delete data->dialog;

#if QT_VERSION >= 0x050400
              if (context) {
                assert (surface);
                context->makeCurrent (surface);
              }
#endif
            }
            delete data;
          }
          p.data = nullptr;
          main_window->setUpdatesEnabled (true);
        }

      }
    }
  }
}



