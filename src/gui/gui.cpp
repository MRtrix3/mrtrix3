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


#include "gui/gui.h"

namespace MR
{
  namespace GUI
  {


    namespace Context
    {
#if QT_VERSION >= 0x050400
        std::pair<QOpenGLContext*,QSurface*> current() {
          QOpenGLContext* context = QOpenGLContext::currentContext();
          QSurface* surface = context ? context->surface() : nullptr;
          return { context, surface };
        }

        std::pair<QOpenGLContext*,QSurface*> get (QWidget* window) {
          QOpenGLContext* context = reinterpret_cast<QOpenGLWidget*> (window)->context();
          QSurface* surface = context ? context->surface() : nullptr;
          return { context, surface };
        }

        std::pair<QOpenGLContext*,QSurface*> makeCurrent (QWidget* window) {
          auto previous_context = current();
          if (window)
            reinterpret_cast<QOpenGLWidget*> (window)->makeCurrent();
          return previous_context;
        }

        void restore (std::pair<QOpenGLContext*,QSurface*> previous_context) {
          if (previous_context.first)
            previous_context.first->makeCurrent (previous_context.second);
        }
#else
        std::pair<int,int> current() { return { 0, 0 }; }
        std::pair<int,int> get (QWidget*) { return { 0, 0 }; }
        std::pair<int,int> makeCurrent (QWidget*) { return { 0, 0 }; }
        void restore (std::pair<int,int>) { }
#endif
    }



    QWidget* App::main_window = nullptr;
    App* App::application = nullptr;

    void App::set_main_window (QWidget* window) {
      main_window = window;
    }


  }
}




