/* Copyright (c) 2008-2025 the MRtrix3 contributors.
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

#pragma once
#define GUI_APP_H

#include <QApplication>
#if QT_VERSION >= QT_VERSION_CHECK(5, 13, 0)
#include <QStyleHints>
#endif

#include "app.h"
#include "file/config.h"
#include "opengl/glutils.h"

#include <functional>

namespace MR::GUI {

inline QString qstr(const std::string &s) { return QString::fromUtf8(s.c_str()); }

class App : public QApplication {

public:
  App(int &cmdline_argc, char **cmdline_argv); // check_syntax off

  // this needs to be defined on a per-application basis:
  virtual bool event(QEvent *event) override;

  static void set_main_window(QWidget *window, GL::Area *glarea) {
    main_window = window;
    GL::glwidget = glarea;
  }

  static QWidget *main_window;
  static App *application;

  static void setEventHandler(std::function<bool(QEvent *)> handler) { App::application->event_handler = handler; }

private:
  std::function<bool(QEvent *)> event_handler;
};

} // namespace MR::GUI
