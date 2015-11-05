/*
    Copyright 2008 Brain Research Institute, Melbourne, Australia

    Written by J-Donald Tournier, 27/06/08.

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

#ifndef __gui_app_h__
#define __gui_app_h__

#include "app.h"
#include "progressbar.h"
#include "file/config.h"
#include "gui/opengl/gl.h"
#include "gui/dialog/progress.h"
#include "gui/dialog/report_exception.h"
#include "gui/dialog/dicom.h"
#include "gui/dialog/file.h"

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
        std::pair<int,int> makeCurrent (QWidget*) { return { 0, 0 }; }
        void restore (std::pair<int,int>) { }
#endif

        struct Grab {
          decltype (current()) previous_context;
          Grab (QWidget* window = nullptr) : previous_context (makeCurrent (window)) { }
          ~Grab () { restore (previous_context); }
        };
    }



    class App : public QObject {
      Q_OBJECT

      public:
        App (int& cmdline_argc, char** cmdline_argv) {
          application = this;
          ::MR::File::Config::init ();
          ::MR::GUI::GL::set_default_context ();

          new QApplication (cmdline_argc, cmdline_argv);
          ::MR::App::init (cmdline_argc, cmdline_argv); 
          qApp->setAttribute (Qt::AA_DontCreateNativeWidgetSiblings);

          ::MR::ProgressInfo::display_func = Dialog::ProgressBar::display;
          ::MR::ProgressInfo::done_func = Dialog::ProgressBar::done;
          ::MR::File::Dicom::select_func = Dialog::select_dicom;
          ::MR::Exception::display_func = Dialog::display_exception;

          ::MR::App::check_overwrite_files_func = Dialog::File::check_overwrite_files_func;
        }

        ~App () {
          delete qApp;
        }

        static void set_main_window (QWidget* window);

        static QWidget* main_window;
        static App* application;

      public slots:
        void startProgressBar ();
        void displayProgressBar (QString text, int value, bool bounded);
        void doneProgressBar ();

    };

  }
}

#endif

