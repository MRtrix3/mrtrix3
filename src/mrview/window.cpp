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

#include <QMessageBox>
#include <QAction>
#include <QMenuBar>
#include <QStatusBar>
#include <QDockWidget>

#include "icon.h"
#include "dialog/file.h"
#include "mrview/glarea.h"
#include "mrview/window.h"

#include "mrview/tool/roi_analysis.h"

namespace MR {
  namespace Viewer {

    Window::Window() { 
      setWindowTitle (tr("MRView"));
      setWindowIcon (get_icon());
      setMinimumSize (256, 256);

      glarea = new GLArea (this);
      setCentralWidget (glarea);


      // Actions:
      open_action = new QAction (tr("&Open"), this);
      open_action->setShortcut (tr("Ctrl+O"));
      open_action->setStatusTip (tr("Open an existing image"));
      connect (open_action, SIGNAL (triggered()), this, SLOT (open()));

      save_action = new QAction(tr("&Save"), this);
      save_action->setShortcut (tr("Ctrl+S"));
      save_action->setStatusTip (tr("Save the current image"));
      connect (save_action, SIGNAL (triggered()), this, SLOT (save()));

      properties_action = new QAction(tr("&Properties"), this);
      properties_action->setStatusTip (tr("Display the properties of the current image"));
      connect (properties_action, SIGNAL (triggered()), this, SLOT (properties()));

      quit_action = new QAction(tr("&Quit"), this);
      quit_action->setShortcut (tr("Ctrl+Q"));
      quit_action->setStatusTip (tr("Exit MRView"));
      connect (quit_action, SIGNAL (triggered()), this, SLOT (close()));


      axial_action = new QAction(tr("&Axial"), this);
      axial_action->setShortcut (tr("A"));
      axial_action->setStatusTip (tr("Switch to axial projection"));
      connect (axial_action, SIGNAL (triggered()), this, SLOT (axial()));

      sagittal_action = new QAction(tr("&Sagittal"), this);
      sagittal_action->setShortcut (tr("S"));
      sagittal_action->setStatusTip (tr("Switch to sagittal projection"));
      connect (sagittal_action, SIGNAL (triggered()), this, SLOT (sagittal()));

      coronal_action = new QAction(tr("&Coronal"), this);
      coronal_action->setShortcut (tr("C"));
      coronal_action->setStatusTip (tr("Switch to coronal projection"));
      connect (coronal_action, SIGNAL (triggered()), this, SLOT (coronal()));

      show_focus_action = new QAction(tr("Show &Focus"), this);
      show_focus_action->setShortcut (tr("Ctrl+F"));
      show_focus_action->setStatusTip (tr("Show focus with the crosshairs"));
      connect (show_focus_action, SIGNAL (triggered()), this, SLOT (show_focus()));

      reset_windowing_action = new QAction(tr("Reset &Windowing"), this);
      reset_windowing_action->setShortcut (tr("R"));
      reset_windowing_action->setStatusTip (tr("Reset image brightness & contrast"));
      connect (reset_windowing_action, SIGNAL (triggered()), this, SLOT (reset_windowing()));

      reset_view_action = new QAction(tr("Reset &View"), this);
      reset_view_action->setShortcut (tr("Crtl+R"));
      reset_view_action->setStatusTip (tr("Reset image projection & zoom"));
      connect (reset_view_action, SIGNAL (triggered()), this, SLOT (reset_view()));

      full_screen_action = new QAction(tr("F&ull Screen"), this);
      full_screen_action->setCheckable (true);
      full_screen_action->setChecked (false);
      full_screen_action->setShortcut (tr("F11"));
      full_screen_action->setStatusTip (tr("Toggle full screen mode"));
      connect (full_screen_action, SIGNAL (triggered()), this, SLOT (full_screen()));

      OpenGL_action = new QAction(tr("&OpenGL Info"), this);
      OpenGL_action->setStatusTip (tr("Display OpenGL information"));
      connect (OpenGL_action, SIGNAL (triggered()), this, SLOT (OpenGL()));

      about_action = new QAction(tr("&About"), this);
      about_action->setStatusTip (tr("Display information about MRView"));
      connect (about_action, SIGNAL (triggered()), this, SLOT (about()));

      aboutQt_action = new QAction(tr("about &Qt"), this);
      aboutQt_action->setStatusTip (tr("Display information about Qt"));
      connect (aboutQt_action, SIGNAL (triggered()), this, SLOT (aboutQt()));


      // Menus:
      file_menu = menuBar()->addMenu (tr("&File"));
      file_menu->addAction (open_action);
      file_menu->addAction (save_action);
      file_menu->addSeparator();
      file_menu->addAction (properties_action);
      file_menu->addSeparator();
      file_menu->addAction (quit_action);

      view_menu = menuBar()->addMenu (tr("&View"));
      view_menu->addAction (axial_action);
      view_menu->addAction (sagittal_action);
      view_menu->addAction (coronal_action);
      view_menu->addSeparator();
      view_menu->addAction (show_focus_action);
      view_menu->addAction (reset_windowing_action);
      view_menu->addAction (reset_view_action);
      view_menu->addAction (full_screen_action);

      tool_menu = menuBar()->addMenu (tr("&Tools"));

      image_menu = menuBar()->addMenu (tr("&Image"));

      menuBar()->addSeparator();

      help_menu = menuBar()->addMenu (tr("&Help"));
      help_menu->addAction (OpenGL_action);
      help_menu->addAction (about_action);
      help_menu->addAction (aboutQt_action);

      // Dock:
      add_tool (new Tool::ROI (this));
      


      // StatusBar:
      statusBar()->showMessage(tr("Ready"));
    }

    void Window::add_tool (Tool::Base* tool) {
      addDockWidget (Qt::RightDockWidgetArea, tool);
      tool_menu->addAction (tool->toggleViewAction());
    }

    void Window::open () { 
      Dialog::File dialog (this, "Select images to open", true, true); 
      if (dialog.exec()) {
        VecPtr<Image::Header> list;
        dialog.get_images (list);
        for (size_t i = 0; i < list.size(); ++i)
          VAR (*list[i]);
      }
    }
    void Window::save () { TEST; }
    void Window::properties () { TEST; }


    void Window::axial () { TEST; }
    void Window::sagittal () { TEST; }
    void Window::coronal () { TEST; }
    void Window::show_focus () { TEST; }
    void Window::reset_windowing () { TEST; }
    void Window::reset_view () { TEST; }
    void Window::full_screen () { if (full_screen_action->isChecked()) showFullScreen(); else showNormal(); }


    void Window::OpenGL () { TEST; }
    void Window::about () { 
      QMessageBox::about(this, tr("About MRView"),
          tr("<b>MRView</b> is the MRtrix viewer"));
    }
    void Window::aboutQt () { QMessageBox::aboutQt (this, tr("About MRView")); }

  }
}


