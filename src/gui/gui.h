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
 * See the Mozilla Public License v. 2.0 for more details.
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


    class App : public QObject { NOMEMALIGN
      Q_OBJECT

      public:
        App (int& cmdline_argc, char** cmdline_argv);

        ~App () {
          delete qApp;
        }

        static void set_main_window (QWidget* window, GL::Area* glarea) {
          main_window = window;
          GL::glwidget = glarea;
        }

        static QWidget* main_window;
        static App* application;
    };



  }
}

#endif

