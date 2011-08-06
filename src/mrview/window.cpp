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

#include "opengl/gl.h"

#include <QMessageBox>
#include <QAction>
#include <QMenuBar>
#include <QStatusBar>
#include <QDockWidget>
#include <QGLWidget>

#include "app.h"
#include "icons.h"
#include "dialog/file.h"
#include "dialog/report_exception.h"
#include "dialog/opengl.h"
#include "dialog/image_properties.h"
#include "mrview/window.h"
#include "mrview/shader.h"
#include "mrview/mode/base.h"
#include "mrview/tool/base.h"
#include "image/header.h"
#include "image/voxel.h"
#include "dataset/copy.h"


namespace MR {
  namespace Viewer {

    class Window::GLArea : public QGLWidget 
    {
      public:
        GLArea (Window& parent) : 
          QGLWidget (QGLFormat (QGL::DoubleBuffer | QGL::DepthBuffer | QGL::StencilBuffer | QGL::Rgba), &parent),
          main (parent) {
            setCursor (Cursor::crosshair); 
            setAutoBufferSwap (false);
            setMouseTracking (true);
            setAcceptDrops (true);
          }

        QSize minimumSizeHint () const { return QSize (512, 512); }
        QSize sizeHint () const { return QSize (512, 512); }

      protected:
        void dragEnterEvent(QDragEnterEvent *event) { event->acceptProposedAction(); }
        void dragMoveEvent(QDragMoveEvent *event) { event->acceptProposedAction(); }
        void dragLeaveEvent(QDragLeaveEvent *event) { event->accept(); }
        void dropEvent(QDropEvent *event) 
        {
          const QMimeData *mimeData = event->mimeData();
          if (mimeData->hasUrls()) {
            VecPtr<MR::Image::Header> list;
            QList<QUrl> urlList = mimeData->urls();
            for (int i = 0; i < urlList.size() && i < 32; ++i) {
              try {
                list.push_back (new MR::Image::Header (urlList.at(i).path().toAscii().constData()));
              }
              catch (Exception& e) {
                Dialog::report_exception (e, &main);
              }
            }
            if (list.size())
              main.add_images (list);
          }
        }

      private:
        Window& main;

        void initializeGL () { main.initGL(); }
        void paintGL () { main.paintGL(); }
        void resizeGL (int width, int height) { main.resizeGL (width, height); }

        void mousePressEvent (QMouseEvent* event) { main.mousePressEventGL (event); }
        void mouseMoveEvent (QMouseEvent* event) { main.mouseMoveEventGL (event); }
        void mouseDoubleClickEvent (QMouseEvent* event) { main.mouseDoubleClickEventGL (event); }
        void mouseReleaseEvent (QMouseEvent* event) { main.mouseReleaseEventGL (event); }
        void wheelEvent (QWheelEvent* event) { main.wheelEventGL (event); }
    };



    Window::Window() : 
      glarea (new GLArea (*this)),
      mode (NULL),
      orient (NAN, NAN, NAN, NAN), 
      field_of_view (100.0),
      proj (2)
    { 
      setWindowTitle (tr("MRView"));
      setCentralWidget (glarea);

      QImage icon_image (Icon::mrtrix.data, Icon::mrtrix.width, Icon::mrtrix.height, QImage::Format_ARGB32);
      setWindowIcon (QPixmap::fromImage (icon_image));

      // File actions:
      open_action = new QAction (tr("&Open"), this);
      open_action->setShortcut (tr("Ctrl+O"));
      open_action->setStatusTip (tr("Open an existing image"));
      connect (open_action, SIGNAL (triggered()), this, SLOT (image_open_slot()));

      save_action = new QAction(tr("&Save"), this);
      save_action->setShortcut (tr("Ctrl+S"));
      save_action->setStatusTip (tr("Save the current image"));
      connect (save_action, SIGNAL (triggered()), this, SLOT (image_save_slot()));

      close_action = new QAction(tr("&Close"), this);
      close_action->setShortcut (tr("Ctrl+W"));
      close_action->setStatusTip (tr("Close the current image"));
      connect (close_action, SIGNAL (triggered()), this, SLOT (image_close_slot()));

      properties_action = new QAction(tr("&Properties"), this);
      properties_action->setStatusTip (tr("Display the properties of the current image"));
      connect (properties_action, SIGNAL (triggered()), this, SLOT (image_properties_slot()));

      quit_action = new QAction(tr("&Quit"), this);
      quit_action->setShortcut (tr("Ctrl+Q"));
      quit_action->setStatusTip (tr("Exit MRView"));
      connect (quit_action, SIGNAL (triggered()), this, SLOT (close()));

      // File menus:
      file_menu = menuBar()->addMenu (tr("&File"));
      file_menu->addAction (open_action);
      file_menu->addAction (save_action);
      file_menu->addAction (close_action);
      file_menu->addSeparator();
      file_menu->addAction (properties_action);
      file_menu->addSeparator();
      file_menu->addAction (quit_action);

      // View actions:
      full_screen_action = new QAction(tr("F&ull Screen"), this);
      full_screen_action->setCheckable (true);
      full_screen_action->setChecked (false);
      full_screen_action->setShortcut (tr("F11"));
      full_screen_action->setStatusTip (tr("Toggle full screen mode"));
      connect (full_screen_action, SIGNAL (triggered()), this, SLOT (full_screen_slot()));

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
        mode_actions[n]->setStatusTip (tr(Mode::tooltip (n)));
        mode_group->addAction (mode_actions[n]);
        view_menu->addAction (mode_actions[n]);
      }
      mode_actions[0]->setChecked (true);
      connect (mode_group, SIGNAL (triggered(QAction*)), this, SLOT (select_mode_slot(QAction*)));
      view_menu->addSeparator();

      view_menu_mode_area = view_menu->addSeparator();
      view_menu_mode_common_area = view_menu->addSeparator();
      view_menu->addAction (full_screen_action);


      // Tool menu:
      tool_menu = menuBar()->addMenu (tr("&Tools"));
      size_t num_tools = Tool::count();
      for (size_t n = 0; n < num_tools; ++n) {
        Tool::Base* tool = Tool::create (*this, n);
        addDockWidget (Qt::RightDockWidgetArea, tool);
        tool_menu->addAction (tool->toggleViewAction());
      }

      // Image menu:

      next_image_action = new QAction(tr("&Next image"), this);
      next_image_action->setShortcut (tr("Tab"));
      next_image_action->setStatusTip (tr("View the next image in the list"));
      connect (next_image_action, SIGNAL (triggered()), this, SLOT (image_next_slot()));

      prev_image_action = new QAction(tr("&Previous image"), this);
      prev_image_action->setShortcut (tr("Shift+Tab"));
      prev_image_action->setStatusTip (tr("View the previous image in the list"));
      connect (prev_image_action, SIGNAL (triggered()), this, SLOT (image_previous_slot()));

      reset_windowing_action = new QAction(tr("Reset &Windowing"), this);
      reset_windowing_action->setShortcut (tr("Home"));
      reset_windowing_action->setStatusTip (tr("Reset image brightness & contrast"));
      connect (reset_windowing_action, SIGNAL (triggered()), this, SLOT (image_reset_slot()));

      image_interpolate_action = new QAction(tr("&Interpolate"), this);
      image_interpolate_action->setShortcut (tr("I"));
      image_interpolate_action->setCheckable (true);
      image_interpolate_action->setChecked (true);
      image_interpolate_action->setStatusTip (tr("Toggle between nearest-neighbour and linear interpolation"));
      connect (image_interpolate_action, SIGNAL (triggered()), this, SLOT (image_interpolate_slot()));

      image_group = new QActionGroup (this);
      image_group->setExclusive (true);
      connect (image_group, SIGNAL (triggered(QAction*)), this, SLOT (image_select_slot(QAction*)));

      image_menu = menuBar()->addMenu (tr("&Image"));
      image_menu->addAction (next_image_action);
      image_menu->addAction (prev_image_action);
      image_menu->addSeparator();
      image_menu->addAction (reset_windowing_action);
      image_menu->addAction (image_interpolate_action);
      colourmap_menu = image_menu->addMenu (tr("&colourmap"));
      image_list_area = image_menu->addSeparator();

      // Colourmap menu:
      ColourMap::init (this, colourmap_group, colourmap_menu, colourmap_actions);
      connect (colourmap_group, SIGNAL (triggered(QAction*)), this, SLOT (select_colourmap_slot()));
      colourmap_menu->addSeparator();
      invert_scale_action = new QAction(tr("&Invert scaling"), this);
      invert_scale_action->setCheckable (true);
      invert_scale_action->setStatusTip (tr("invert the current scaling"));
      connect (invert_scale_action, SIGNAL (changed()), this, SLOT (select_colourmap_slot()));
      colourmap_menu->addAction (invert_scale_action);

      invert_colourmap_action = new QAction(tr("Invert &Colourmap"), this);
      invert_colourmap_action->setCheckable (true);
      invert_colourmap_action->setStatusTip (tr("invert the current colourmap"));
      connect (invert_colourmap_action, SIGNAL (changed()), this, SLOT (select_colourmap_slot()));
      colourmap_menu->addAction (invert_colourmap_action);

      menuBar()->addSeparator();


      // Help actions:
      OpenGL_action = new QAction(tr("&OpenGL Info"), this);
      OpenGL_action->setStatusTip (tr("Display OpenGL information"));
      connect (OpenGL_action, SIGNAL (triggered()), this, SLOT (OpenGL_slot()));

      about_action = new QAction(tr("&About"), this);
      about_action->setStatusTip (tr("Display information about MRView"));
      connect (about_action, SIGNAL (triggered()), this, SLOT (about_slot()));

      aboutQt_action = new QAction(tr("about &Qt"), this);
      aboutQt_action->setStatusTip (tr("Display information about Qt"));
      connect (aboutQt_action, SIGNAL (triggered()), this, SLOT (aboutQt_slot()));

      // Help menu:
      help_menu = menuBar()->addMenu (tr("&Help"));
      help_menu->addAction (OpenGL_action);
      help_menu->addAction (about_action);
      help_menu->addAction (aboutQt_action);

      set_image_menu ();
    }




    Window::~Window () 
    {
      delete glarea;
      delete [] mode_actions;
      delete [] colourmap_actions;
    }







    void Window::image_open_slot () 
    { 
      Dialog::File dialog (this, "Select images to open", true, true); 
      if (dialog.exec()) {
        VecPtr<MR::Image::Header> list;
        dialog.get_images (list);
        add_images (list);
      }
    }



    void Window::add_images (VecPtr<MR::Image::Header>& list)
    {
      for (size_t i = 0; i < list.size(); ++i) {
        QAction* action = new Image (*this, list.release (i));
        image_group->addAction (action);
        if (!i) image_select_slot (action);
      }
      set_image_menu();
    }



    void Window::image_save_slot () 
    { 
      Dialog::File dialog (this, "Select image destination", false, false); 
      if (dialog.exec()) {
        std::vector<std::string> selection;
        dialog.get_selection (selection);
        if (selection.size() != 1) return;
        try {
          current_image()->header().create (selection[0]);
          MR::Image::Voxel<float> dest (current_image()->header());
          DataSet::copy_with_progress (dest, current_image()->vox);
        }
        catch (Exception& E) {
          Dialog::report_exception (E, this);
        }
      }
    }




    void Window::image_close_slot ()
    {
      Image* image = current_image();
      assert (image);
      QList<QAction*> list = image_group->actions();
      if (list.size() > 1) {
        for (int n = 0; n < list.size(); ++n) {
          if (image == list[n]) {
            image_select_slot (list[(n+1)%list.size()]);
            break;
          }
        }
      }
      image_group->removeAction (image);
      delete image;
      set_image_menu();
    }




    void Window::image_properties_slot () 
    {
      assert (current_image()); 
      Dialog::ImageProperties props (this, current_image()->header());
      props.exec(); 
    }



    void Window::select_mode_slot (QAction* action) 
    {
      delete mode;
      size_t n = 0;
      while (action != mode_actions[n]) {
        assert (Mode::name (n) != NULL);
        ++n;
      }
      mode = Mode::create (*this, n);
      mode->updateGL();
    }




    void Window::select_colourmap_slot () 
    {
      Image* image = current_image();
      if (image) {
        QAction* action = colourmap_group->checkedAction();
        size_t n = 0;
        while (action != colourmap_actions[n]) 
          ++n;
        image->set_colourmap (
            ColourMap::from_menu (n), 
            invert_scale_action->isChecked(), 
            invert_colourmap_action->isChecked());
        mode->updateGL();
      }
    }




    void Window::image_reset_slot ()
    { 
      Image* image = current_image();
      if (image) {
        image->reset_windowing(); 
        mode->updateGL();
      }
    }



    void Window::image_interpolate_slot ()
    { 
      Image* image = current_image();
      if (image) {
        image->set_interpolate (image_interpolate_action->isChecked());
        mode->updateGL();
      }
    }


    void Window::full_screen_slot ()
    { 
      if (full_screen_action->isChecked()) 
        showFullScreen();
      else
        showNormal();
    }




    void Window::image_next_slot () 
    { 
      QAction* action = image_group->checkedAction();
      QList<QAction*> list = image_group->actions();
      for (int n = 0; n < list.size(); ++n) {
        if (action == list[n]) {
          image_select_slot (list[(n+1)%list.size()]);
          return;
        }
      }
    }




    void Window::image_previous_slot ()
    { 
      QAction* action = image_group->checkedAction();
      QList<QAction*> list = image_group->actions();
      for (int n = 0; n < list.size(); ++n) {
        if (action == list[n]) {
          image_select_slot (list[(n+list.size()-1)%list.size()]);
          return;
        }
      }
    }




    inline void Window::set_image_menu ()
    {
      int N = image_group->actions().size();
      next_image_action->setEnabled (N>1);
      prev_image_action->setEnabled (N>1);
      reset_windowing_action->setEnabled (N>0);
      colourmap_menu->setEnabled (N>0);
      save_action->setEnabled (N>0);
      close_action->setEnabled (N>0);
      properties_action->setEnabled (N>0);
      glarea->updateGL();
    }





    void Window::image_select_slot (QAction* action)
    {
      action->setChecked (true); 
      image_interpolate_action->setChecked (current_image()->interpolate());
      glarea->updateGL();
    }


    void Window::OpenGL_slot () 
    {
      Dialog::OpenGL gl (this);
      gl.exec(); 
    }




    void Window::about_slot () { 
      std::string message = printf ("<h1>MRView</h1>The MRtrix viewer, version %zu.%zu.%zu<br>"
          "<em>%d bit %s version, built " __DATE__ "</em><p>"
          "Author: %s<p><em>%s</em>",
          App::version[0], App::version[1], App::version[2], int(8*sizeof(size_t)), 
#ifdef NDEBUG
          "release"
#else
          "debug"
#endif
          , App::author, App::copyright);

      QMessageBox::about(this, tr("About MRView"), message.c_str());
    }




    void Window::aboutQt_slot () { QMessageBox::aboutQt (this); }




    inline void Window::paintGL () 
    {
      if (mode->in_paint())
        return;

      glDrawBuffer (GL_BACK);
      mode->paintGL(); 

      // blit back buffer to front buffer.
      // we avoid flipping to guarantee back buffer is unchanged and can be
      // re-used for incremental updates.
      glReadBuffer (GL_BACK);
      glDrawBuffer (GL_FRONT);

      glBindFramebuffer (GL_READ_FRAMEBUFFER, 0);
      glBindFramebuffer (GL_DRAW_FRAMEBUFFER, 0);

      glBlitFramebuffer (
          0, 0, width(), height(), 
          0, 0, width(), height(), 
          GL_COLOR_BUFFER_BIT, GL_NEAREST);
      glFlush();

      DEBUG_OPENGL;
    }




    inline void Window::initGL () 
    { 
      GL::init ();

      CHECK_GL_EXTENSION (ARB_fragment_shader);
      CHECK_GL_EXTENSION (ARB_vertex_shader);
      CHECK_GL_EXTENSION (ARB_geometry_shader4);
      CHECK_GL_EXTENSION (EXT_texture3D);
      CHECK_GL_EXTENSION (ARB_texture_non_power_of_two);
      CHECK_GL_EXTENSION (ARB_vertex_buffer_object);
      CHECK_GL_EXTENSION (ARB_pixel_buffer_object);
      CHECK_GL_EXTENSION (ARB_framebuffer_object);

      GLint max_num;
      glGetIntegerv (GL_MAX_GEOMETRY_OUTPUT_VERTICES_ARB, &max_num);
      info ("maximum number of vertices for geometry shader: " + str(max_num));

      glClearColor (0.0, 0.0, 0.0, 0.0);
      glEnable (GL_DEPTH_TEST);

      mode = Mode::create (*this, 0);
      DEBUG_OPENGL;
    }



    inline void Window::resizeGL (int width, int height) 
    {
      glViewport (0, 0, width, height); 
    }

    inline void Window::mousePressEventGL (QMouseEvent* event) 
    {
      assert (mode);
      if (current_image())
        mode->mousePressEvent (event);
    }

    inline void Window::mouseMoveEventGL (QMouseEvent* event) 
    {
      assert (mode);
      if (current_image())
        mode->mouseMoveEvent (event);
    }

    inline void Window::mouseDoubleClickEventGL (QMouseEvent* event) 
    {
      assert (mode);
      if (current_image())
        mode->mouseDoubleClickEvent (event);
    }

    inline void Window::mouseReleaseEventGL (QMouseEvent* event) 
    {
      assert (mode);
      if (current_image())
        mode->mouseReleaseEvent (event);
    }

    inline void Window::wheelEventGL (QWheelEvent* event) 
    {
      assert (mode);
      if (current_image())
        mode->wheelEvent (event);
    }





  }
}



