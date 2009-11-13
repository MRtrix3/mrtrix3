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

#include <GL/glu.h>
#include "mrview/pane.h"
#include "mrview/window.h"

namespace MR {
  namespace Viewer {

    Pane::GLArea::GLArea (Pane& parent) : pane (parent)
    {
      add_events (Gdk::BUTTON_MOTION_MASK | Gdk::BUTTON_PRESS_MASK | Gdk::BUTTON_RELEASE_MASK | Gdk::SCROLL_MASK);
      GdkGLConfig* glconfig = gdk_gl_config_new_by_mode (GdkGLConfigMode (GDK_GL_MODE_RGB | GDK_GL_MODE_DEPTH | GDK_GL_MODE_ALPHA | GDK_GL_MODE_DOUBLE));
      if (!glconfig) { error ("failed to initialise OpenGL!"); return; }
      gtk_widget_set_gl_capability (GTK_WIDGET (gobj()), glconfig, NULL, true, GDK_GL_RGBA_TYPE);
    }




    void Pane::GLArea::on_realize ()
    {
      Gtk::DrawingArea::on_realize();
      if (start()) return;
      glClearColor (0.0, 0.0, 0.0, 0.0);
      if (pane.mode) pane.mode->realize();
      end();
    }



    bool Pane::GLArea::on_configure_event (GdkEventConfigure* event) 
    { 
      if (start()) return (false);
      glViewport (0, 0, get_width(), get_height());
      pane.set_viewport();

      if (pane.mode) pane.mode->configure();
      end();
      return (true);
    }



    bool Pane::GLArea::on_expose_event (GdkEventExpose* event)
    { 
      if (start()) return (false);
      glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

      Slice::Current S (pane);
      if (S.image) {
        pane.render.update (S);
        if (pane.mode) {
          pane.mode->draw();
          for (std::vector<SideBar::Base*>::iterator i = pane.sidebar.begin(); i != pane.sidebar.end(); ++i)
            (*i)->draw();
        }
      }

      GLenum error_code = glGetError();
      if (error_code != GL_NO_ERROR) 
        error (std::string ("OpenGL Error: ") + (const char*) gluErrorString (error_code));

      swap();
      end();

      if (&pane == &Window::Main->pane()) Window::Main->update_statusbar(); 
      return (true);
    }





    Pane::Pane () : 
      FOV (NAN),
      gl_area (*this) 
    { 
      set_mode (0);
      set_shadow_type (Gtk::SHADOW_IN); 
      set_size_request (256, 256);
      add (gl_area);
      show_all(); 
    }


    bool Pane::on_key_press (GdkEventKey* event) 
    { 
      if (!mode) return (false);
      if (mode->on_key_press (event)) return (true); 
      if (!Window::Main->sidebar_displayed()) return (false);
      for (std::vector<SideBar::Base*>::iterator i = sidebar.begin(); i != sidebar.end(); ++i) 
        if ((*i)->on_key_press (event)) return (true);
      return (false);
    }

    bool Pane::on_button_press (GdkEventButton* event) 
    { 
      if (!mode) return (false);
      if (mode->on_button_press (event)) return (true); 
      if (!Window::Main->sidebar_displayed()) return (false);
      for (std::vector<SideBar::Base*>::iterator i = sidebar.begin(); i != sidebar.end(); ++i) 
        if ((*i)->on_button_press (event)) return (true);
      return (false);
    }

    bool Pane::on_button_release (GdkEventButton* event)
    { 
      if (!mode) return (false);
      if (mode->on_button_release (event)) return (true); 
      if (!Window::Main->sidebar_displayed()) return (false);
      for (std::vector<SideBar::Base*>::iterator i = sidebar.begin(); i != sidebar.end(); ++i) 
        if ((*i)->on_button_release (event)) return (true);
      return (false);
    }

    bool Pane::on_motion (GdkEventMotion* event)
    { 
      if (!mode) return (false);
      if (mode->on_motion (event)) return (true); 
      if (!Window::Main->sidebar_displayed()) return (false);
      for (std::vector<SideBar::Base*>::iterator i = sidebar.begin(); i != sidebar.end(); ++i) 
        if ((*i)->on_motion (event)) return (true);
      return (false);
    }

    bool Pane::on_scroll (GdkEventScroll* event)
    { 
      if (!mode) return (false);
      if (mode->on_scroll (event)) return (true); 
      if (!Window::Main->sidebar_displayed()) return (false);
      for (std::vector<SideBar::Base*>::iterator i = sidebar.begin(); i != sidebar.end(); ++i) 
        if ((*i)->on_scroll (event)) return (true);
      return (false);
    }




  }
}



