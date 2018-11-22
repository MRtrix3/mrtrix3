/* Copyright (c) 2008-2019 the MRtrix3 contributors.
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
 * See the Mozila Public License v. 2.0 for more details.
 *
 * For more details, see http://www.mrtrix.org/.
 */

#ifndef __gui_app_h__
#define __gui_app_h__

#include "app.h"
#include "file/config.h"
#include "gui/opengl/gl.h"

namespace MR
{
  namespace GUI
  {



    namespace Context
    {
#if QT_VERSION >= 0x050400
        std::pair<QOpenGLContext*,QSurface*> current();
        std::pair<QOpenGLContext*,QSurface*> get (QWidget*);
        std::pair<QOpenGLContext*,QSurface*> makeCurrent (QWidget*);
        void restore (std::pair<QOpenGLContext*,QSurface*>);
#else
        std::pair<int,int> current();
        std::pair<int,int> get (QWidget*);
        std::pair<int,int> makeCurrent (QWidget*);
        void restore (std::pair<int,int>);
#endif


      struct Grab { NOMEMALIGN
        decltype (current()) previous_context;
        Grab (QWidget* window = nullptr) : previous_context (makeCurrent (window)) { }
        ~Grab () { restore (previous_context); }
      };
    }



    class App : public QObject { NOMEMALIGN
      Q_OBJECT

      public:
        App (int& cmdline_argc, char** cmdline_argv);

        ~App () {
          delete qApp;
        }

        static void set_main_window (QWidget* window);

        static QWidget* main_window;
        static App* application;
    };

#ifndef NDEBUG
# define ASSERT_GL_CONTEXT_IS_CURRENT(window) { \
  auto __current_context = ::MR::GUI::Context::current(); \
  auto __expected_context = ::MR::GUI::Context::get (window); \
  assert (__current_context == __expected_context); \
}
#else
# define ASSERT_GL_CONTEXT_IS_CURRENT(window)
#endif


  }
}

#endif

