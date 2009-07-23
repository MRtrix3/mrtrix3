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


    24-10-2008 J-Donald Tournier <d.tournier@brain.org.au>
    * add functions to normalise plot amplitude

*/

#ifndef __dwi_render_frame_h__
#define __dwi_render_frame_h__

#include <gtk/gtkgl.h>
#include <gtkmm/drawingarea.h>

#include "use_gl.h"
#include "dwi/renderer.h"

#define MAX_LOD 8

namespace MR {
  namespace DWI {

    class RenderFrame : public Gtk::DrawingArea 
    {
      public:
        RenderFrame ();

        float ambient, diffuse, specular, shine, color[3], lightpos[4], background[3];
        void  reset_lighting ();
        void  do_reset_lighting ();

        void set (const std::vector<float>& new_values);

        void set_rotation (const GLfloat* rotation = NULL)       { rotation_matrix = rotation; refresh(); }

        void set_show_axes (bool yesno = true)         { show_axes = yesno; refresh(); }
        void set_hide_neg_lobes (bool yesno = true)    { hide_neg_lobes = yesno; values_changed = true; refresh(); }
        void set_color_by_dir (bool yesno = true)      { color_by_dir = yesno; refresh(); }
        void set_use_lighting (bool yesno = true)      { use_lighting = yesno; refresh(); }
        void set_normalise (bool yesno = true)         { normalise = yesno; refresh(); }
        void set_LOD (int num)                         { if (lod == num) return; lod = num; lmax_or_lod_changed = true; refresh(); } 
        void set_lmax (int num)                        { if (lmax == num) return; lmax = num; lmax_or_lod_changed = true; refresh(); } 

        int  get_LOD () const                          { return (lod); }
        int  get_lmax () const                         { return (lmax); }
        float get_scale () const                       { return (scale); }
        bool get_show_axes () const                    { return (show_axes); }
        bool get_hide_neg_lobes () const               { return (hide_neg_lobes); }
        bool get_color_by_dir () const                 { return (color_by_dir); }
        bool get_use_lighting () const                 { return (use_lighting); }
        bool get_normalise () const                    { return (normalise); }


      protected:
        float view_angle, distance, elevation, azimuth, line_width, scale;
        int   lod, lmax;
        bool  show_axes, hide_neg_lobes, color_by_dir, use_lighting, lmax_or_lod_changed, values_changed, normalise;
        double old_x, old_y;
        const GLfloat* rotation_matrix;

        Renderer renderer;
        std::vector<float> values;

        void on_realize ();
        bool on_configure_event (GdkEventConfigure* event);
        bool on_expose_event (GdkEventExpose* event);
        bool on_button_press_event (GdkEventButton* event);
        bool on_button_release_event (GdkEventButton* event);
        bool on_motion_notify_event (GdkEventMotion* event);
        bool on_scroll_event (GdkEventScroll* event);

        void  refresh () { queue_draw(); }
        bool  start () { return (!gdk_gl_drawable_gl_begin (gtk_widget_get_gl_drawable (GTK_WIDGET (gobj())), gtk_widget_get_gl_context (GTK_WIDGET (gobj())))); }
        void  end() { gdk_gl_drawable_gl_end (gtk_widget_get_gl_drawable (GTK_WIDGET (gobj()))); }
        void  swap () {
          GdkGLDrawable* gldrawable = gtk_widget_get_gl_drawable (GTK_WIDGET (gobj()));
          if (gdk_gl_drawable_is_double_buffered (gldrawable)) gdk_gl_drawable_swap_buffers (gldrawable); 
          else glFlush (); 
        }
    };


  }
}

#endif


