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
class QActionGroup;

namespace MR {
  namespace Viewer {

    namespace Mode { class Base; }
    namespace Tool { class Base; }

    class Window : public QMainWindow
    {
      Q_OBJECT

      public:
        Window();
        ~Window();

      private slots:
        void open ();
        void save ();
        void properties ();

        void select_mode (QAction* action);
        void reset_windowing ();
        void full_screen ();

        void OpenGL ();
        void about ();
        void aboutQt ();

        QPoint global_position (const QPoint& position) const;

      private:
        class GLArea;

        GLArea *glarea;
        Mode::Base* mode;

        QMenu *file_menu, *view_menu, *tool_menu, *image_menu, *help_menu;
        QAction *open_action, *save_action, *properties_action, *quit_action;
        QAction *view_menu_mode_area, *reset_windowing_action, *full_screen_action;
        QAction **mode_actions;
        QAction *OpenGL_action, *about_action, *aboutQt_action;
        QActionGroup *mode_group;

        void add_tool (Tool::Base* tool);

        void paintGL ();
        void initGL ();
        void mousePressEventGL (QMouseEvent* event);
        void mouseMoveEventGL (QMouseEvent* event);
        void mouseDoubleClickEventGL (QMouseEvent* event);
        void mouseReleaseEventGL (QMouseEvent* event);
        void wheelEventGL (QWheelEvent* event);

        friend class Mode::Base;
        friend class Window::GLArea;
    };

  }
}

#endif
