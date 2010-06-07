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
    * scale plot by SH(0,0) coefficient when normalise option is set

*/

#include <fstream>
#include <GL/glu.h>
#include <glibmm/stringutils.h>
#include <gdkmm/cursor.h>
#include <gtkmm/main.h>
#include <gtkmm/container.h>
#include <gsl/gsl_sf_legendre.h>

#include "dwi/render_frame.h"

#define D2R 0.01745329252
#define AZ_INC 0.2
#define EL_INC 0.2
#define DIST_INC 0.005

#define DIST_MIN 0.1
#define DIST_MAX 10.0

#define SCALE_INC 1.05
#define SCALE_MIN 0.01
#define SCALE_MAX 10.0

#define ANGLE_INC 0.1
#define ANGLE_MIN 1.0
#define ANGLE_MAX 90.0


namespace MR {
  namespace DWI {

    RenderFrame::RenderFrame() :
      ambient (0.4), diffuse (0.7), specular (0.3), shine (5.0), 
      view_angle (40.0), distance (0.3), elevation (0.0), azimuth (0.0), line_width (1.0), scale (1.0), 
      lod (3), lmax (12), 
      show_axes (true), hide_neg_lobes (true), color_by_dir (true), use_lighting (true), 
      lmax_or_lod_changed (true), values_changed (true),
      rotation_matrix (NULL)
    {
      set_size_request (150, 150);
      background[0] = background[1] = background[2] = 1.0;
      lightpos[0] = 1.0; lightpos[1] = 1.0; lightpos[2] = 3.0; lightpos[3] = 0.0;
      color[0] = 1.0; color[1] = 1.0; color[2] = 0.0;

      add_events (Gdk::BUTTON_MOTION_MASK | Gdk::BUTTON_PRESS_MASK | Gdk::BUTTON_RELEASE_MASK | Gdk::SCROLL_MASK);
      GdkGLConfig* glconfig = gdk_gl_config_new_by_mode (GdkGLConfigMode (GDK_GL_MODE_RGB | GDK_GL_MODE_DEPTH | GDK_GL_MODE_DOUBLE));
      if (!glconfig) { error ("failed to initialise OpenGL!"); return; }
      gtk_widget_set_gl_capability (GTK_WIDGET (gobj()), glconfig, NULL, true, GDK_GL_RGBA_TYPE);
    }


    void RenderFrame::set (const std::vector<float>& new_values)
    {
      values_changed = true;
      values = new_values;
      refresh();
    }


    void RenderFrame::on_realize ()
    {
      Gtk::DrawingArea::on_realize();
      if (start()) return;
      glEnable (GL_DEPTH_TEST);
      glClearColor (background[0], background[1], background[2], 0.0);
      do_reset_lighting ();
      info ("DWI renderer successfully initialised");
      info ("GL_RENDERER   = " + std::string ((const char*) glGetString (GL_RENDERER)));
      info ("GL_VERSION    = " + std::string ((const char*) glGetString (GL_VERSION)));
      info ("GL_VENDOR     = " + std::string ((const char*) glGetString (GL_VENDOR)));
      end();
    }



    bool RenderFrame::on_configure_event (GdkEventConfigure* event) 
    { 
      if (start()) return (false);
      glViewport (0, 0, get_width(), get_height());

      end();
      return (true);
    }



    bool RenderFrame::on_expose_event (GdkEventExpose* event)
    { 
      if (start()) return (false);
      glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

      float dist (1.0 / (distance * view_angle * D2R));
      float ratio = float (get_width())/float (get_height());

      float a (ratio < 1.0 ? view_angle/ratio : view_angle);
      glMatrixMode (GL_PROJECTION);
      glLoadIdentity ();
      gluPerspective (a, ratio, ( dist-3.0 > 0.001 ? dist-3.0 : 0.001 ), dist+3.0);
      glMatrixMode (GL_MODELVIEW);

      glLoadIdentity ();
      glTranslatef (0.0, 0.0, -dist);
      if (rotation_matrix) {
        glMultMatrixf (rotation_matrix);
      }
      else {
        glRotatef (elevation, 1.0, 0.0, 0.0);
        glRotatef (azimuth, 0.0, 0.0, 1.0);
      }
      glDepthMask (GL_TRUE);

      if (values.size()) {
        glPushMatrix();
        glDisable (GL_BLEND);
        if (lmax_or_lod_changed) { renderer.precompute (lmax, lod, get_toplevel()->get_window()); values_changed = true; }
        if (values_changed) renderer.calculate (values, lmax, hide_neg_lobes);

        lmax_or_lod_changed = values_changed = false;

        if (use_lighting) glEnable (GL_LIGHTING);
        GLfloat v[] = { 0.9, 0.9, 0.9, 1.0 }; 
        glMaterialfv (GL_BACK, GL_AMBIENT_AND_DIFFUSE, v);


        float s (scale);
        if (normalise) s /= values[0];
        glScalef (s, s, s); 

        renderer.draw (use_lighting, color_by_dir ? NULL : color);

        if (use_lighting) glDisable (GL_LIGHTING);

        glPopMatrix();
      }

      glLineWidth (line_width);
      glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
      glEnable (GL_BLEND);
      glEnable (GL_LINE_SMOOTH);

      if (show_axes) {
        glColor3f (1.0, 0.0, 0.0);
        glBegin (GL_LINES);
        glVertex3f (-1.0, -1.0, -1.0);
        glVertex3f (1.0, -1.0, -1.0);
        glEnd();

        glColor3f (0.0, 1.0, 0.0);
        glBegin (GL_LINES);
        glVertex3f (-1.0, -1.0, -1.0);
        glVertex3f (-1.0, 1.0, -1.0);
        glEnd ();

        glColor3f (0.0, 0.0, 1.0);
        glBegin (GL_LINES);
        glVertex3f (-1.0, -1.0, -1.0);
        glVertex3f (-1.0, -1.0, 1.0);
        glEnd ();
      }

      glDisable (GL_BLEND);
      glDisable (GL_LINE_SMOOTH);

      GLenum error_code = glGetError();
      if (error_code != GL_NO_ERROR) 
        error (std::string ("OpenGL Error: ") + (const char*) gluErrorString (error_code));

      swap();
      end();
      return (true);
    }





    void RenderFrame::reset_lighting ()
    {
      if (start()) return;
      glClearColor (background[0], background[1], background[2], 0.0);
      do_reset_lighting();
      end();
      refresh();
    }




    void RenderFrame::do_reset_lighting ()
    {
      glLightModeli (GL_LIGHT_MODEL_TWO_SIDE, GL_TRUE);
      glShadeModel (GL_SMOOTH);
      glEnable (GL_LIGHT0);
      glEnable (GL_NORMALIZE);

      GLfloat v[] = { ambient, ambient, ambient, 1.0 };
      glLightModelfv (GL_LIGHT_MODEL_AMBIENT, v);

      v[0] = v[1] = v[2] = specular;
      glMaterialfv (GL_FRONT, GL_SPECULAR, v);

      glMaterialf (GL_FRONT, GL_SHININESS, shine);
      glLightfv (GL_LIGHT0, GL_POSITION, lightpos);

      v[0] = v[1] = v[2] = diffuse;
      glLightfv (GL_LIGHT0, GL_DIFFUSE, v);

      v[0] = v[1] = v[2] = 1.0;
      glLightfv (GL_LIGHT0, GL_SPECULAR, v);

      v[0] = v[1] = v[2] = 0.0;
      glLightfv (GL_LIGHT0, GL_AMBIENT, v);
    }



    bool RenderFrame::on_button_press_event (GdkEventButton* event)
    {
      old_x = event->x;
      old_y = event->y;
      return (false);
    }



    bool RenderFrame::on_button_release_event (GdkEventButton* event) { return (false); }




    bool RenderFrame::on_motion_notify_event (GdkEventMotion* event) 
    {
      double inc_x = event->x - old_x;
      double inc_y = event->y - old_y;
      old_x = event->x;
      old_y = event->y;

      if ((event->state & MODIFIERS) == GDK_BUTTON1_MASK) {
        azimuth += AZ_INC*inc_x;
        if (azimuth >= 360.0) azimuth -= 360.0;
        if (azimuth < 0.0) azimuth += 360.0;
        elevation += EL_INC*inc_y;
        if (elevation < -180.0) elevation = -180.0;
        if (elevation > 0.0) elevation = 0.0;
        refresh();
        return (true);
      }

      if ((event->state & MODIFIERS) == GDK_BUTTON3_MASK) {
        distance *= 1.0 - DIST_INC*inc_y;
        if (distance < DIST_MIN) distance = DIST_MIN;
        if (distance > DIST_MAX) distance = DIST_MAX;
        refresh();
        return (true);
      }

      if ((event->state & MODIFIERS) == (GDK_BUTTON3_MASK | GDK_CONTROL_MASK)) {
        view_angle -= ANGLE_INC*inc_y;
        if (view_angle < ANGLE_MIN) view_angle = ANGLE_MIN;
        if (view_angle > ANGLE_MAX) view_angle = ANGLE_MAX;
        refresh();
        return (true);
      }

      return (false); 
    }




    bool RenderFrame::on_scroll_event (GdkEventScroll* event) 
    {
      if (event->direction == GDK_SCROLL_UP) {
        scale *= SCALE_INC;
        if (scale > SCALE_MAX) scale = SCALE_MAX;
      }
      else if (event->direction == GDK_SCROLL_DOWN) {
        scale /= SCALE_INC;
        if (scale < SCALE_MIN) scale = SCALE_MIN;
      }
      refresh();
      return (true); 
    }



  }
}





