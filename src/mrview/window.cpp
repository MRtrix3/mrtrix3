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

#include "data_type.h"
#include "opengl/gl.h"

#include <QMessageBox>
#include <QAction>
#include <QMenuBar>
#include <QStatusBar>
#include <QDockWidget>
#include <QGLWidget>

#include "icon.h"
#include "dialog/file.h"
#include "mrview/window.h"
#include "mrview/mode/base.h"

#include "mrview/tool/roi_analysis.h"

namespace MR {
  namespace Viewer {

    class Window::GLArea : public QGLWidget {
      public:
        GLArea (Window& parent);
        ~GLArea ();
        QSize minimumSizeHint () const;
        QSize sizeHint () const;

      private:
        Window& main;

        void initializeGL ();
        void paintGL ();
        void resizeGL (int width, int height);

        void mousePressEvent (QMouseEvent* event);
        void mouseMoveEvent (QMouseEvent* event);
        void mouseDoubleClickEvent (QMouseEvent* event);
        void mouseReleaseEvent (QMouseEvent* event);
        void wheelEvent (QWheelEvent* event);
    };



    Window::Window() : 
      glarea (new GLArea (*this)),
      mode (NULL) 
    { 
      setWindowTitle (tr("MRView"));
      setWindowIcon (get_icon());
      setMinimumSize (256, 256);
      setCentralWidget (glarea);


      // File actions:
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

      // File menus:
      file_menu = menuBar()->addMenu (tr("&File"));
      file_menu->addAction (open_action);
      file_menu->addAction (save_action);
      file_menu->addSeparator();
      file_menu->addAction (properties_action);
      file_menu->addSeparator();
      file_menu->addAction (quit_action);

      // View actions:
      reset_windowing_action = new QAction(tr("Reset &Windowing"), this);
      reset_windowing_action->setShortcut (tr("R"));
      reset_windowing_action->setStatusTip (tr("Reset image brightness & contrast"));
      connect (reset_windowing_action, SIGNAL (triggered()), this, SLOT (reset_windowing()));
/*
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
      show_focus_action->setShortcut (tr("F"));
      show_focus_action->setStatusTip (tr("Show focus with the crosshairs"));
      connect (show_focus_action, SIGNAL (triggered()), this, SLOT (show_focus()));

      reset_view_action = new QAction(tr("Reset &View"), this);
      reset_view_action->setShortcut (tr("Crtl+R"));
      reset_view_action->setStatusTip (tr("Reset image projection & zoom"));
      connect (reset_view_action, SIGNAL (triggered()), this, SLOT (reset_view()));
*/
      full_screen_action = new QAction(tr("F&ull Screen"), this);
      full_screen_action->setCheckable (true);
      full_screen_action->setChecked (false);
      full_screen_action->setShortcut (tr("F11"));
      full_screen_action->setStatusTip (tr("Toggle full screen mode"));
      connect (full_screen_action, SIGNAL (triggered()), this, SLOT (full_screen()));

      // View menu:
      view_menu = menuBar()->addMenu (tr("&View"));
      size_t num_modes;
      for (num_modes = 0; Mode::name (num_modes); ++num_modes);
      assert (num_modes > 1);
      mode_actions = new QAction* [num_modes];
      mode_group = new QActionGroup (this);
      mode_group->setExclusive (true);

      for (size_t n = 0; n < num_modes; ++n) {
        mode_actions[n] = new QAction (tr(Mode::name (n)), this);
        mode_actions[n]->setCheckable (num_modes > 1);
        mode_actions[n]->setShortcut (tr(std::string ("F"+str(n+1)).c_str()));
        mode_group->addAction (mode_actions[n]);
        view_menu->addAction (mode_actions[n]);
      }
      mode_actions[0]->setChecked (true);
      connect (mode_group, SIGNAL (triggered(QAction*)), this, SLOT (select_mode(QAction*)));
      view_menu->addSeparator();

      view_menu_mode_area = view_menu->addSeparator();
      view_menu->addAction (reset_windowing_action);
      view_menu->addSeparator();

      //view_menu->addAction (axial_action);
      //view_menu->addAction (sagittal_action);
      //view_menu->addAction (coronal_action);
      //view_menu->addAction (show_focus_action);
      //view_menu->addAction (reset_view_action);
      view_menu->addSeparator();
      view_menu->addAction (full_screen_action);


      // Tool & Image menus:
      tool_menu = menuBar()->addMenu (tr("&Tools"));

      image_menu = menuBar()->addMenu (tr("&Image"));

      menuBar()->addSeparator();


      // Help actions:
      OpenGL_action = new QAction(tr("&OpenGL Info"), this);
      OpenGL_action->setStatusTip (tr("Display OpenGL information"));
      connect (OpenGL_action, SIGNAL (triggered()), this, SLOT (OpenGL()));

      about_action = new QAction(tr("&About"), this);
      about_action->setStatusTip (tr("Display information about MRView"));
      connect (about_action, SIGNAL (triggered()), this, SLOT (about()));

      aboutQt_action = new QAction(tr("about &Qt"), this);
      aboutQt_action->setStatusTip (tr("Display information about Qt"));
      connect (aboutQt_action, SIGNAL (triggered()), this, SLOT (aboutQt()));

      // Help menu:
      help_menu = menuBar()->addMenu (tr("&Help"));
      help_menu->addAction (OpenGL_action);
      help_menu->addAction (about_action);
      help_menu->addAction (aboutQt_action);


      // Dock:
      add_tool (new Tool::ROI (this));
      

      // StatusBar:
      statusBar()->showMessage(tr("Ready"));

      select_mode (mode_actions[0]);
    }




    Window::~Window () 
    {
      delete glarea;
    }





    QPoint Window::global_position (const QPoint& position) const { return (glarea->mapToGlobal (position)); }



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

    void Window::select_mode (QAction* action) 
    {
      delete mode;
      size_t n = 0;
      while (action != mode_actions[n]) {
        assert (Mode::name (n) != NULL);
        ++n;
      }
      mode = Mode::create (*this, n);
    }

    void Window::reset_windowing () { TEST; }
    void Window::full_screen () { if (full_screen_action->isChecked()) showFullScreen(); else showNormal(); }


    void Window::OpenGL () { TEST; }
    void Window::about () { 
      QMessageBox::about(this, tr("About MRView"),
          tr("<b>MRView</b> is the MRtrix viewer"));
    }
    void Window::aboutQt () { QMessageBox::aboutQt (this, tr("About MRView")); }

    inline void Window::paintGL () { mode->paint(); }
    inline void Window::initGL () { };
    inline void Window::mousePressEventGL (QMouseEvent* event) { mode->mousePressEvent (event); }
    inline void Window::mouseMoveEventGL (QMouseEvent* event) { mode->mouseMoveEvent (event); }
    inline void Window::mouseDoubleClickEventGL (QMouseEvent* event) { mode->mouseDoubleClickEvent (event); }
    inline void Window::mouseReleaseEventGL (QMouseEvent* event) { mode->mouseReleaseEvent (event); }
    inline void Window::wheelEventGL (QWheelEvent* event) { mode->wheelEvent (event); }




    Window::GLArea::GLArea (Window& parent) : 
      QGLWidget (QGLFormat (QGL::DoubleBuffer | QGL::DepthBuffer | QGL::Rgba), &parent),
      main (parent)
    { }

    Window::GLArea::~GLArea () { }


    QSize Window::GLArea::minimumSizeHint() const { return QSize (256, 256); }
    QSize Window::GLArea::sizeHint() const { return QSize (256, 256); }

    void Window::GLArea::initializeGL () 
    {
      GL::init ();

      CHECK_GL_EXTENSION (ARB_fragment_shader);
      CHECK_GL_EXTENSION (ARB_vertex_shader);
      CHECK_GL_EXTENSION (ARB_geometry_shader4);
      CHECK_GL_EXTENSION (EXT_texture3D);
      CHECK_GL_EXTENSION (ARB_texture_non_power_of_two);
      CHECK_GL_EXTENSION (ARB_framebuffer_object);

      glClearColor (0.0, 0.0, 0.0, 0.0);
      glEnable (GL_DEPTH_TEST);

      main.initGL();
    }

    void Window::GLArea::paintGL () 
    {
      glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
      glLoadIdentity();
      main.paintGL();
    }

    void Window::GLArea::resizeGL (int width, int height) { glViewport (0, 0, width, height); }
    void Window::GLArea::mousePressEvent (QMouseEvent *event) { main.mousePressEventGL (event); }
    void Window::GLArea::mouseMoveEvent (QMouseEvent *event) { main.mouseMoveEventGL (event); }
    void Window::GLArea::mouseDoubleClickEvent (QMouseEvent* event) { main.mouseDoubleClickEventGL (event); }
    void Window::GLArea::mouseReleaseEvent (QMouseEvent* event) { main.mouseReleaseEventGL (event); }
    void Window::GLArea::wheelEvent (QWheelEvent* event) { main.wheelEventGL (event); }

  }
}



