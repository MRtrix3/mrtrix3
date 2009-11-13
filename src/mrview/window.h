/*
   Copyright 2009 Brain Research Institute, Melbourne, Australia

   Written by J-Donald Tournier, 13/11/09.

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

#ifndef __viewer_window_h__
#define __viewer_window_h__

#include <QMainWindow>

class QMenu;
class QAction;

namespace MR {
  namespace Viewer {

    class GLArea;
    namespace Tool { class Base; }

    class Window : public QMainWindow
    {
      Q_OBJECT

      public:
        Window();

      private slots:
        void open ();
        void save ();
        void properties ();

        void axial ();
        void sagittal ();
        void coronal ();
        void show_focus ();
        void reset_windowing ();
        void reset_view ();
        void full_screen ();

        void OpenGL ();
        void about ();
        void aboutQt ();

      private:
        QMenu *file_menu, *view_menu, *tool_menu, *image_menu, *help_menu;
        QAction *open_action, *save_action, *properties_action, *quit_action;
        QAction *axial_action, *sagittal_action, *coronal_action, *show_focus_action, *reset_windowing_action, *reset_view_action, *full_screen_action;
        QAction *OpenGL_action, *about_action, *aboutQt_action;
        GLArea *glarea;

        void add_tool (Tool::Base* tool);
    };

  }
}

#endif
