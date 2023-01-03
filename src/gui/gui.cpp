/* Copyright (c) 2008-2023 the MRtrix3 contributors.
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

#include <locale>
#include <clocale>
#include "gui/gui.h"
#include "gui/opengl/gl.h"

namespace MR
{
  namespace GUI
  {



    QWidget* App::main_window = nullptr;
    App* App::application = nullptr;



    App::App (int& cmdline_argc, char** cmdline_argv) :
      QApplication (cmdline_argc, cmdline_argv)
    {
      application = this;
      ::MR::File::Config::init ();
      ::MR::GUI::GL::set_default_context ();

      QLocale::setDefault(QLocale::c());
      std::locale::global (std::locale::classic());
      std::setlocale (LC_ALL, "C");

      setAttribute (Qt::AA_DontCreateNativeWidgetSiblings);
    }



  }
}




