#include "gui/opengl/gl.h"

#include <QMessageBox>
#include <QAction>
#include <QMenuBar>
#include <QStatusBar>
#include <QDockWidget>
#include <QGLWidget>

#include "app.h"
#include "image/header.h"
#include "image/voxel.h"
#include "image/copy.h"
#include "gui/icons.h"
#include "gui/dialog/file.h"
#include "gui/dialog/opengl.h"
#include "gui/dialog/image_properties.h"
#include "gui/mrview/window.h"
#include "gui/mrview/shader.h"
#include "gui/mrview/mode/base.h"
#include "gui/mrview/mode/list.h"
#include "gui/mrview/tool/base.h"
#include "gui/mrview/tool/list.h"


namespace MR
{
  namespace GUI
  {
    namespace MRView
    {
      using namespace App;

#define MODE(classname, specifier, name, description) #specifier ", "
#define MODE_OPTION(classname, specifier, name, description) MODE(classname, specifier, name, description)

      const OptionGroup Window::options = OptionGroup ("General options")
        + Option ("mode", "select initial display mode by its short ID. Valid mode IDs are: "
#include "gui/mrview/mode/list.h"
            )
        + Argument ("name");
#undef MODE
#undef MODE_OPTION


      class Window::GLArea : public QGLWidget
      {
        public:
          GLArea (Window& parent) :
            QGLWidget (QGLFormat (QGL::DoubleBuffer | QGL::DepthBuffer | QGL::StencilBuffer | QGL::Rgba), &parent),
            main (parent) {
              setCursor (Cursor::crosshair);
              setMouseTracking (true);
              setAcceptDrops (true);
              setFocusPolicy (Qt::StrongFocus);
            }

          QSize minimumSizeHint () const {
            return QSize (512, 512);
          }
          QSize sizeHint () const {
            return QSize (512, 512);
          }

        protected:
          void dragEnterEvent (QDragEnterEvent* event) {
            event->acceptProposedAction();
          }
          void dragMoveEvent (QDragMoveEvent* event) {
            event->acceptProposedAction();
          }
          void dragLeaveEvent (QDragLeaveEvent* event) {
            event->accept();
          }
          void dropEvent (QDropEvent* event) {
            const QMimeData* mimeData = event->mimeData();
            if (mimeData->hasUrls()) {
              VecPtr<MR::Image::Header> list;
              QList<QUrl> urlList = mimeData->urls();
              for (int i = 0; i < urlList.size() && i < 32; ++i) {
                try {
                  list.push_back (new MR::Image::Header (urlList.at (i).path().toAscii().constData()));
                }
                catch (Exception& e) {
                  e.display();
                }
              }
              if (list.size())
                main.add_images (list);
            }
          }

        private:
          Window& main;

          void initializeGL () {
            main.initGL();
          }
          void paintGL () {
            main.paintGL();
          }
          void mousePressEvent (QMouseEvent* event) {
            main.mousePressEventGL (event);
          }
          void mouseMoveEvent (QMouseEvent* event) {
            main.mouseMoveEventGL (event);
          }
          void mouseDoubleClickEvent (QMouseEvent* event) {
            main.mouseDoubleClickEventGL (event);
          }
          void mouseReleaseEvent (QMouseEvent* event) {
            main.mouseReleaseEventGL (event);
          }
          void wheelEvent (QWheelEvent* event) {
            main.wheelEventGL (event);
          }
      };



      Window::Window() :
        glarea (new GLArea (*this)),
        mode (NULL),
        orient (NAN, NAN, NAN, NAN),
        field_of_view (100.0),
        proj (2)
      {
        setWindowTitle (tr ("MRView"));
        setCentralWidget (glarea);

        QImage icon_image (Icon::mrtrix.data, Icon::mrtrix.width, Icon::mrtrix.height, QImage::Format_ARGB32);
        setWindowIcon (QPixmap::fromImage (icon_image));

        // File actions:
        open_action = new QAction (tr ("&Open"), this);
        open_action->setShortcut (tr ("Ctrl+O"));
        open_action->setStatusTip (tr ("Open an existing image"));
        connect (open_action, SIGNAL (triggered()), this, SLOT (image_open_slot()));

        save_action = new QAction (tr ("&Save"), this);
        save_action->setShortcut (tr ("Ctrl+S"));
        save_action->setStatusTip (tr ("Save the current image"));
        connect (save_action, SIGNAL (triggered()), this, SLOT (image_save_slot()));

        close_action = new QAction (tr ("&Close"), this);
        close_action->setShortcut (tr ("Ctrl+W"));
        close_action->setStatusTip (tr ("Close the current image"));
        connect (close_action, SIGNAL (triggered()), this, SLOT (image_close_slot()));

        properties_action = new QAction (tr ("&Properties"), this);
        properties_action->setStatusTip (tr ("Display the properties of the current image"));
        connect (properties_action, SIGNAL (triggered()), this, SLOT (image_properties_slot()));

        quit_action = new QAction (tr ("&Quit"), this);
        quit_action->setShortcut (tr ("Ctrl+Q"));
        quit_action->setStatusTip (tr ("Exit MRView"));
        connect (quit_action, SIGNAL (triggered()), this, SLOT (close()));

        // File menus:
        file_menu = menuBar()->addMenu (tr ("&File"));
        file_menu->addAction (open_action);
        file_menu->addAction (save_action);
        file_menu->addAction (close_action);
        file_menu->addSeparator();
        file_menu->addAction (properties_action);
        file_menu->addSeparator();
        file_menu->addAction (quit_action);

        // View actions:
        full_screen_action = new QAction (tr ("F&ull Screen"), this);
        full_screen_action->setCheckable (true);
        full_screen_action->setChecked (false);
        full_screen_action->setShortcut (tr ("F11"));
        full_screen_action->setStatusTip (tr ("Toggle full screen mode"));
        connect (full_screen_action, SIGNAL (triggered()), this, SLOT (full_screen_slot()));

        // View menu:
        view_menu = menuBar()->addMenu (tr ("&View"));

        mode_group = new QActionGroup (this);
        mode_group->setExclusive (true);

#define MODE(classname, specifier, name, description) \
        view_menu->addAction (new Action<classname> (mode_group, #name, #description, n++));
#define MODE_OPTION(classname, specifier, name, description) MODE(classname, specifier, name, description)
        {
          using namespace Mode;
          size_t n = 1;
#include "gui/mrview/mode/list.h"
        }
#undef MODE
#undef MODE_OPTION

        mode_group->actions()[0]->setChecked (true);
        connect (mode_group, SIGNAL (triggered (QAction*)), this, SLOT (select_mode_slot (QAction*)));
        view_menu->addSeparator();

        view_menu_mode_area = view_menu->addSeparator();
        view_menu_mode_common_area = view_menu->addSeparator();
        view_menu->addAction (full_screen_action);


        // Tool menu:
        tool_menu = menuBar()->addMenu (tr ("&Tools"));
        tool_group = new QActionGroup (this);
        tool_group->setExclusive (false);

#undef TOOL
#define TOOL(classname, name, description) \
        tool_menu->addAction (new Action<classname> (tool_group, #name, #description, n++));
#define TOOL_OPTION(classname, name, description) TOOL(classname, name, description)
        {
          using namespace Tool;
          size_t n = 1;
#include "gui/mrview/tool/list.h"
        }
        connect (tool_group, SIGNAL (triggered (QAction*)), this, SLOT (select_tool_slot (QAction*)));

        // Image menu:

        next_image_action = new QAction (tr ("&Next image"), this);
        next_image_action->setShortcut (tr ("Tab"));
        next_image_action->setStatusTip (tr ("View the next image in the list"));
        connect (next_image_action, SIGNAL (triggered()), this, SLOT (image_next_slot()));

        prev_image_action = new QAction (tr ("&Previous image"), this);
        prev_image_action->setShortcut (tr ("Shift+Tab"));
        prev_image_action->setStatusTip (tr ("View the previous image in the list"));
        connect (prev_image_action, SIGNAL (triggered()), this, SLOT (image_previous_slot()));

        next_image_volume_action = new QAction (tr ("Next volume"), this);
        next_image_volume_action->setShortcut (tr ("Right"));
        next_image_volume_action->setStatusTip (tr ("View the next volume in the image (4th dimension)"));
        connect (next_image_volume_action, SIGNAL (triggered()), this, SLOT (image_next_volume_slot()));

        prev_image_volume_action = new QAction (tr ("Previous volume"), this);
        prev_image_volume_action->setShortcut (tr ("Left"));
        prev_image_volume_action->setStatusTip (tr ("View the previous volume in the image (4th dimension)"));
        connect (prev_image_volume_action, SIGNAL (triggered()), this, SLOT (image_previous_volume_slot()));

        next_image_volume_group_action = new QAction (tr ("Next volume group"), this);
        next_image_volume_group_action->setShortcut (tr ("Shift+Right"));
        next_image_volume_group_action->setStatusTip (tr ("View the next group of volumes in the image (5th dimension)"));
        connect (next_image_volume_group_action, SIGNAL (triggered()), this, SLOT (image_next_volume_group_slot()));

        prev_image_volume_group_action = new QAction (tr ("Previous volume group"), this);
        prev_image_volume_group_action->setShortcut (tr ("Shift+Left"));
        prev_image_volume_group_action->setStatusTip (tr ("View the previous group of volumes in the image (5th dimension)"));
        connect (prev_image_volume_group_action, SIGNAL (triggered()), this, SLOT (image_previous_volume_group_slot()));

        reset_windowing_action = new QAction (tr ("Reset &Windowing"), this);
        reset_windowing_action->setShortcut (tr ("Home"));
        reset_windowing_action->setStatusTip (tr ("Reset image brightness & contrast"));
        connect (reset_windowing_action, SIGNAL (triggered()), this, SLOT (image_reset_slot()));

        image_interpolate_action = new QAction (tr ("&Interpolate"), this);
        image_interpolate_action->setShortcut (tr ("I"));
        image_interpolate_action->setCheckable (true);
        image_interpolate_action->setChecked (true);
        image_interpolate_action->setStatusTip (tr ("Toggle between nearest-neighbour and linear interpolation"));
        connect (image_interpolate_action, SIGNAL (triggered()), this, SLOT (image_interpolate_slot()));

        image_group = new QActionGroup (this);
        image_group->setExclusive (true);
        connect (image_group, SIGNAL (triggered (QAction*)), this, SLOT (image_select_slot (QAction*)));

        image_menu = menuBar()->addMenu (tr ("&Image"));
        image_menu->addAction (next_image_action);
        image_menu->addAction (prev_image_action);
        image_menu->addSeparator();
        image_menu->addAction (reset_windowing_action);
        image_menu->addAction (image_interpolate_action);
        colourmap_menu = image_menu->addMenu (tr ("&colourmap"));
        image_menu->addSeparator();
        image_menu->addAction (next_image_volume_action);
        image_menu->addAction (prev_image_volume_action);
        image_menu->addAction (next_image_volume_group_action);
        image_menu->addAction (prev_image_volume_group_action);
        image_list_area = image_menu->addSeparator();

        // Colourmap menu:
        ColourMap::init (this, colourmap_group, colourmap_menu, colourmap_actions);
        connect (colourmap_group, SIGNAL (triggered (QAction*)), this, SLOT (select_colourmap_slot()));
        colourmap_menu->addSeparator();
        invert_scale_action = new QAction (tr ("&Invert scaling"), this);
        invert_scale_action->setCheckable (true);
        invert_scale_action->setStatusTip (tr ("invert the current scaling"));
        connect (invert_scale_action, SIGNAL (changed()), this, SLOT (select_colourmap_slot()));
        colourmap_menu->addAction (invert_scale_action);

        invert_colourmap_action = new QAction (tr ("Invert &Colourmap"), this);
        invert_colourmap_action->setCheckable (true);
        invert_colourmap_action->setStatusTip (tr ("invert the current colourmap"));
        connect (invert_colourmap_action, SIGNAL (changed()), this, SLOT (select_colourmap_slot()));
        colourmap_menu->addAction (invert_colourmap_action);

        menuBar()->addSeparator();


        // Help actions:
        OpenGL_action = new QAction (tr ("&OpenGL Info"), this);
        OpenGL_action->setStatusTip (tr ("Display OpenGL information"));
        connect (OpenGL_action, SIGNAL (triggered()), this, SLOT (OpenGL_slot()));

        about_action = new QAction (tr ("&About"), this);
        about_action->setStatusTip (tr ("Display information about MRView"));
        connect (about_action, SIGNAL (triggered()), this, SLOT (about_slot()));

        aboutQt_action = new QAction (tr ("about &Qt"), this);
        aboutQt_action->setStatusTip (tr ("Display information about Qt"));
        connect (aboutQt_action, SIGNAL (triggered()), this, SLOT (aboutQt_slot()));

        // Help menu:
        help_menu = menuBar()->addMenu (tr ("&Help"));
        help_menu->addAction (OpenGL_action);
        help_menu->addAction (about_action);
        help_menu->addAction (aboutQt_action);

        set_image_menu ();
      }




      Window::~Window ()
      {
        mode = NULL;
        delete glarea;
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
          QAction* action = new Image (*this, *list[i]);
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
            MR::Image::Buffer<cfloat> dest (selection[0], image()->header());
            MR::Image::Buffer<cfloat>::voxel_type vox (dest);
            MR::Image::copy_with_progress (image()->voxel(), vox);
          }
          catch (Exception& E) {
            E.display();
          }
        }
      }




      void Window::image_close_slot ()
      {
        Image* imagep = image();
        assert (imagep);
        QList<QAction*> list = image_group->actions();
        if (list.size() > 1) {
          for (int n = 0; n < list.size(); ++n) {
            if (imagep == list[n]) {
              image_select_slot (list[ (n+1) %list.size()]);
              break;
            }
          }
        }
        image_group->removeAction (imagep);
        delete imagep;
        set_image_menu();
      }




      void Window::image_properties_slot ()
      {
        assert (image());
        Dialog::ImageProperties props (this, image()->header());
        props.exec();
      }



      void Window::select_mode_slot (QAction* action)
      {
        mode = dynamic_cast<GUI::MRView::Mode::__Action__*> (action)->create (*this);
        mode->updateGL();
      }





      void Window::select_tool_slot (QAction* action)
      {
        Tool::Dock* tool = dynamic_cast<Tool::__Action__*>(action)->instance;
        if (!tool) {
          tool = dynamic_cast<Tool::__Action__*>(action)->create (*this);
          addDockWidget (Qt::RightDockWidgetArea, tool);
          tool->show();
          connect (tool, SIGNAL (visibilityChanged (bool)), action, SLOT (setChecked (bool)));
        }

        if (action->isChecked())
          tool->show();
        else 
          tool->close();
        mode->updateGL();
      }




      void Window::select_colourmap_slot ()
      {
        Image* imagep = image();
        if (imagep) {
          QAction* action = colourmap_group->checkedAction();
          size_t n = 0;
          while (action != colourmap_actions[n])
            ++n;
          imagep->set_colourmap (
            ColourMap::from_menu (n),
            invert_scale_action->isChecked(),
            invert_colourmap_action->isChecked());
          mode->updateGL();
        }
      }




      void Window::image_reset_slot ()
      {
        Image* imagep = image();
        if (imagep) {
          imagep->reset_windowing();
          mode->updateGL();
        }
      }



      void Window::image_interpolate_slot ()
      {
        Image* imagep = image();
        if (imagep) {
          imagep->set_interpolate (image_interpolate_action->isChecked());
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
        int N = image_group->actions().size();
        int n = image_group->actions().indexOf (action);
        image_select_slot (image_group->actions()[(n+1)%N]);
      }




      void Window::image_previous_slot ()
      {
        QAction* action = image_group->checkedAction();
        int N = image_group->actions().size();
        int n = image_group->actions().indexOf (action);
        image_select_slot (image_group->actions()[(n+N-1)%N]);
      }




      void Window::image_next_volume_slot () 
      {
        assert (image());
        ++image()->interp[3];
        set_image_navigation_menu();
        glarea->updateGL();
      }




      void Window::image_previous_volume_slot ()
      {
        assert (image());
        --image()->interp[3];
        set_image_navigation_menu();
        glarea->updateGL();
      }




      void Window::image_next_volume_group_slot () 
      {
        assert (image());
        ++image()->interp[4];
        set_image_navigation_menu();
        glarea->updateGL();
      }




      void Window::image_previous_volume_group_slot ()
      {
        assert (image());
        --image()->interp[4];
        set_image_navigation_menu();
        glarea->updateGL();
      }




      void Window::image_select_slot (QAction* action)
      {
        action->setChecked (true);
        image_interpolate_action->setChecked (image()->interpolate());
        colourmap_group->actions()[image()->colourmap_index()]->setChecked (true);
        invert_scale_action->setChecked (image()->scale_inverted());
        invert_colourmap_action->setChecked (image()->colourmap_inverted());
        setWindowTitle (image()->interp.name().c_str());
        set_image_navigation_menu();
        glarea->updateGL();
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
        set_image_navigation_menu();
        glarea->updateGL();
      }



      inline void Window::set_image_navigation_menu ()
      {
        bool show_next_volume (false), show_prev_volume (false);
        bool show_next_volume_group (false), show_prev_volume_group (false);
        Image* imagep = image();
        if (imagep) {
          if (imagep->interp.ndim() > 3) {
            if (imagep->interp[3] > 0) 
              show_prev_volume = true;
            if (imagep->interp[3] < imagep->interp.dim(3)-1) 
              show_next_volume = true;

            if (imagep->interp.ndim() > 4) {
              if (imagep->interp[4] > 0) 
                show_prev_volume_group = true;
              if (imagep->interp[4] < imagep->interp.dim(4)-1) 
                show_next_volume_group = true;
            }
          }
        }
        prev_image_volume_action->setEnabled (show_prev_volume);
        next_image_volume_action->setEnabled (show_next_volume);
        prev_image_volume_group_action->setEnabled (show_prev_volume_group);
        next_image_volume_group_action->setEnabled (show_next_volume_group);
      }





      void Window::OpenGL_slot ()
      {
        Dialog::OpenGL gl (this);
        gl.exec();
      }




      void Window::about_slot ()
      {
        std::string message = printf ("<h1>MRView</h1>The MRtrix viewer, version %zu.%zu.%zu<br>"
                                      "<em>%d bit %s version, built " __DATE__ "</em><p>"
                                      "Author: %s<p><em>%s</em>",
                                      App::VERSION[0], App::VERSION[1], App::VERSION[2], int (8*sizeof (size_t)),
#ifdef NDEBUG
                                      "release"
#else
                                      "debug"
#endif
                                      , App::AUTHOR, App::COPYRIGHT);

        QMessageBox::about (this, tr ("About MRView"), message.c_str());
      }




      void Window::aboutQt_slot ()
      {
        QMessageBox::aboutQt (this);
      }




      inline void Window::paintGL ()
      {
        glEnable (GL_MULTISAMPLE);
        if (mode->in_paint())
          return;

        glDrawBuffer (GL_BACK);
        mode->paintGL();

/*
        // blit back buffer to front buffer.
        // we avoid flipping to guarantee back buffer is unchanged and can be
        // re-used for incremental updates.

        glBindFramebuffer (GL_READ_FRAMEBUFFER, 0);
        glBindFramebuffer (GL_DRAW_FRAMEBUFFER, 0);

        glReadBuffer (GL_BACK);
        glDrawBuffer (GL_FRONT);

        glBlitFramebuffer (
          0, 0, width(), height(),
          0, 0, width(), height(),
          GL_COLOR_BUFFER_BIT, GL_NEAREST);
        glFlush();
*/
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
        CHECK_GL_EXTENSION (ARB_multisample);

        GLint max_num;
        glGetIntegerv (GL_MAX_GEOMETRY_OUTPUT_VERTICES_ARB, &max_num);
        inform ("maximum number of vertices for geometry shader: " + str (max_num));

        glClearColor (0.0, 0.0, 0.0, 0.0);
        glEnable (GL_DEPTH_TEST);

        Options opt = get_options ("mode");
        if (opt.size()) {
#undef MODE
#define MODE(classname, specifier, name, description) \
   #specifier,
#define MODE_OPTION(classname, specifier, name, description) MODE(classname, specifier, name, description)
          const char* specifier_list[] = { 
#include "gui/mrview/mode/list.h"
            NULL
          };
#undef MODE
#undef MODE_OPTION
          std::string specifier (lowercase (opt[0][0]));
          for (int n = 0; specifier_list[n]; ++n) {
            if (specifier == lowercase (specifier_list[n])) {
              mode = dynamic_cast<Mode::__Action__*> (mode_group->actions()[n])->create (*this);
              goto mode_selected;
            }
          }
          throw Exception ("unknown mode \"" + std::string (opt[0][0]) + "\"");
        }
        else 
          mode = dynamic_cast<Mode::__Action__*> (mode_group->actions()[0])->create (*this);

mode_selected:
        DEBUG_OPENGL;
      }



      inline void Window::mousePressEventGL (QMouseEvent* event)
      {
        assert (mode);
        if (image())
          mode->mousePressEvent (event);
      }

      inline void Window::mouseMoveEventGL (QMouseEvent* event)
      {
        assert (mode);
        if (image())
          mode->mouseMoveEvent (event);
      }

      inline void Window::mouseDoubleClickEventGL (QMouseEvent* event)
      {
        assert (mode);
        if (image())
          mode->mouseDoubleClickEvent (event);
      }

      inline void Window::mouseReleaseEventGL (QMouseEvent* event)
      {
        assert (mode);
        if (image())
          mode->mouseReleaseEvent (event);
      }

      inline void Window::wheelEventGL (QWheelEvent* event)
      {
        assert (mode);
        if (image())
          mode->wheelEvent (event);
      }





    }
  }
}



