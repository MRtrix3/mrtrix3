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

#ifndef __mrview_pane_h__
#define __mrview_pane_h__

#include <GL/glu.h>
#include <gtk/gtkgl.h>
#include <gtkmm/frame.h>
#include <gtkmm/drawingarea.h>

#include "use_gl.h"
#include "point.h"
#include "ptr.h"
#include "mrview/slice.h"
#include "mrview/mode/base.h"
#include "mrview/sidebar/base.h"

namespace MR {
  namespace Viewer {

    class Pane : public Gtk::Frame
    {
      public:
        Pane ();

        void  set_mode (uint index) { mode = Mode::create (*this, index); }   
        bool  check_focus () { /* TODO */ return (false); }

        bool  gl_start () { return (gl_area.start()); }
        void  gl_end() { gl_area.end(); }

        Slice::Info slice;
        Slice::Source source;
        Slice::Renderer render;

        RefPtr<Mode::Base> mode;

        float  FOV;
        Point  focus;

        void  set_viewport () { glGetIntegerv (GL_VIEWPORT, viewport); }
        void  set_projection () { glGetDoublev (GL_MODELVIEW_MATRIX, modelview); glGetDoublev (GL_PROJECTION_MATRIX, projection); }

        Point   model_to_screen (const Point& pos) const
        {
          double wx, wy, wz;
          gluProject (pos[0], pos[1], pos[2], modelview, projection, viewport, &wx, &wy, &wz);
          return (Point (wx, wy, wz));
        }

        Point   screen_to_model (const Point& pos) const
        {
          double wx, wy, wz;
          gluUnProject (pos[0], pos[1], pos[2], modelview, projection, viewport, &wx, &wy, &wz);
          return (Point (wx, wy, wz));
        }

        bool    on_key_press (GdkEventKey* event);
        bool    on_button_press (GdkEventButton* event);
        bool    on_button_release (GdkEventButton* event);
        bool    on_motion (GdkEventMotion* event);
        bool    on_scroll (GdkEventScroll* event);

        const GLdouble* get_modelview () const { return (modelview); }
        const GLdouble* get_projection () const { return (projection); }
        const GLint*    get_viewport () const { return (viewport); }
        GLint           width () const { return (viewport[2]); }
        GLint           height () const { return (viewport[3]); }

        void  activate (SideBar::Base* sidebar_entry) { sidebar.push_back (sidebar_entry); std::sort (sidebar.begin(), sidebar.end(), SideBar::Base::sort_func); }

      protected:
        class GLArea : public Gtk::DrawingArea {
          protected:
            Pane& pane;
            void on_realize ();
            bool on_configure_event (GdkEventConfigure* event);
            bool on_expose_event (GdkEventExpose* event);
            bool on_button_press_event (GdkEventButton* event) { return (pane.on_button_press (event)); }
            bool on_button_release_event (GdkEventButton* event) { return (pane.on_button_release (event)); }
            bool on_motion_notify_event (GdkEventMotion* event) { return (pane.on_motion (event)); }
            bool on_scroll_event (GdkEventScroll* event) { return (pane.on_scroll (event)); }

          public:
            GLArea (Pane& parent);
            bool  start () { return (!gdk_gl_drawable_gl_begin (gtk_widget_get_gl_drawable (GTK_WIDGET (gobj())), gtk_widget_get_gl_context (GTK_WIDGET (gobj())))); }
            void  end() { gdk_gl_drawable_gl_end (gtk_widget_get_gl_drawable (GTK_WIDGET (gobj()))); }
            void  swap () {
              GdkGLDrawable* gldrawable = gtk_widget_get_gl_drawable (GTK_WIDGET (gobj()));
              if (gdk_gl_drawable_is_double_buffered (gldrawable)) gdk_gl_drawable_swap_buffers (gldrawable); 
              else glFlush (); 
            }
        };

        GLArea gl_area;
        GLdouble   modelview[16], projection[16];
        GLint      viewport[4];
        float      prev_FOV;
        Point      prev_focus;
        std::vector<SideBar::Base*> sidebar;

        void  do_update () 
        { 
          if (FOV != prev_FOV || focus != prev_focus) force_update();
          else {
            const Slice::Current S (*this);
            if (slice != S) force_update(); 
          }
        }
        void  force_update () { gl_area.queue_draw(); }

        friend class GLArea;
        friend class DisplayArea;
    };







  }
}

#endif




