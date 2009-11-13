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

#include <gdk/gdkkeysyms.h>

#include "mrview/pane.h"
#include "mrview/image.h"
#include "mrview/window.h"
#include "mrview/slice.h"
#include "mrview/mode/normal.h"

#define ZOOM_MULTIPLIER 0.1
#define ROTATION_INC 0.002


namespace MR {
  namespace Viewer {
    namespace Mode {


      Normal::~Normal ()    { }





      void Normal::configure () 
      {
      }




      void Normal::draw () 
      { 
        const Slice::Current S (pane);
        if (!pane.focus) pane.focus = S.focus;
        if (isnan (pane.FOV)) {
          int ix, iy;
          Slice::get_fixed_axes (S.projection, ix, iy);
          MR::Image::Interp& I (*S.image->interp);
          pane.FOV = ( I.dim(ix)*I.vox(ix) + I.dim(iy)*I.vox(iy) ) / 2.0;
        }

        int w = pane.width(), h = pane.height();
        float fov = pane.FOV / (float) (w+h);
        float wfov = 2.0*w*fov/float(OS);
        float hfov = 2.0*h*fov/float(OS);
        float xfov = (float(OS_x)-0.5*float(OS))*wfov;
        float yfov = (float(OS_y)-0.5*float(OS))*hfov;

        glMatrixMode (GL_PROJECTION);
        glLoadIdentity ();
        glOrtho (xfov, xfov+wfov, yfov, yfov+hfov, -pane.FOV, pane.FOV);
        glMatrixMode (GL_MODELVIEW);

        glLoadIdentity ();
        glMultMatrixf (pane.render.projection_matrix());

        Point f (pane.focus);
        pane.render.focus_to_image_plane (f);
        glTranslatef (f[0], f[1], f[2]);

        pane.set_projection();

        glDisable(GL_BLEND);
        glEnable (GL_TEXTURE_2D);
        glShadeModel (GL_FLAT);
        glDisable (GL_DEPTH_TEST);
        glTexEnvf (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
        glDepthMask (GL_FALSE);
        glColorMask (GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);

        pane.render.draw();

        glDisable (GL_TEXTURE_2D);

        if (Window::Main->show_focus()) {
          Point F = pane.model_to_screen (S.focus);

          glMatrixMode (GL_PROJECTION);
          glPushMatrix ();
          glLoadIdentity ();
          glOrtho (0, w, 0, h, -1.0, 1.0);
          glMatrixMode (GL_MODELVIEW);
          glPushMatrix ();
          glLoadIdentity ();

          float alpha = 0.5;

          glColor4f (1.0, 1.0, 0.0, alpha);
          glLineWidth (1.0);
          glEnable (GL_BLEND);
          glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

          glBegin (GL_LINES);
          glVertex2f (0.0, F[1]);
          glVertex2f (w, F[1]);
          glVertex2f (F[0], 0.0);
          glVertex2f (F[0], h);
          glEnd ();

          glDisable(GL_BLEND);
          glPopMatrix ();
          glMatrixMode (GL_PROJECTION);
          glPopMatrix ();
          glMatrixMode (GL_MODELVIEW);

        }
        glDepthMask (GL_TRUE);
      }





      bool Normal::on_button_press (GdkEventButton* event) 
      {
        xprev = event->x;
        yprev = event->y;
        Slice::Current S (pane);

        if (!(event->state & MODIFIERS) && event->type == GDK_BUTTON_PRESS && event->button == 1) {
          set_focus (S, event->x, event->y);
          return (true);
        }

        return (false); 
      }




      bool Normal::on_button_release (GdkEventButton* event) { return (false); }




      bool Normal::on_motion (GdkEventMotion* event) 
      {
        Slice::Current S (pane);
        if (!S.image) return (false);

        double incx = event->x - xprev;
        double incy = event->y - yprev;
        xprev = event->x;
        yprev = event->y;

        if ((event->state & MODIFIERS) == GDK_BUTTON1_MASK) {
          set_focus (S, event->x, event->y);
          return (true);
        }

        if ((event->state & MODIFIERS) == GDK_BUTTON2_MASK) {
          Point pos = pane.model_to_screen (pane.focus);
          pos[0] -= incx;
          pos[1] += incy;
          pane.focus = pane.screen_to_model (pos);
          Window::Main->update();
          return (true); 
        }

        if ((event->state & MODIFIERS) == GDK_BUTTON3_MASK) {
          S.scaling.adjust (incx, incy);
          Window::Main->update();
          return (true);
        }

        if (S.orientation) { 
          if (incx != 0.0 || incy != 0.0) {
            Point v;
            float angle = 0.0;
            const GLdouble* M = pane.get_modelview();

            if ((event->state & MODIFIERS) == ( GDK_BUTTON1_MASK | GDK_CONTROL_MASK )) {
              v.set (M[2], M[6], M[10]);
              Point pos = pane.model_to_screen (pane.focus);
              float dx = event->x - 0.5*pane.width();
              float dy = event->y - 0.5*pane.height();
              angle = -atan2 (dy, dx);
              angle += atan2 (dy + incy, dx + incx);
            }
            else if ((event->state & MODIFIERS) == ( GDK_BUTTON2_MASK | GDK_CONTROL_MASK )) {
              Point x (-incx*M[0] + incy*M[1], -incx*M[4] + incy*M[5], -incx*M[8] + incy*M[9]);
              Point z (M[2], M[6], M[10]);
              v = x.cross (z);
              angle = ROTATION_INC * v.norm();
            }
            else return (true);

            v.normalise();
            if (angle > M_PI_2) angle = M_PI_2;

            Math::Quaternion rot (angle, v.get());
            S.orientation = rot * S.orientation;
            Window::Main->update();
          }

          return (true);
        }

        return (false); 
      }




      bool Normal::on_scroll (GdkEventScroll* event) 
      { 
        Slice::Current S (pane);
        if (!S.image) return (false);

        if (!(event->state & MODIFIERS) || (event->state & MODIFIERS) == GDK_SHIFT_MASK) {
          float dir = (event->state & MODIFIERS) == GDK_SHIFT_MASK ? 5.0 : 1.0;
          if (event->direction == GDK_SCROLL_DOWN) dir = -dir;
          else if (event->direction != GDK_SCROLL_UP) return (false);

          move_slice (S, dir);
          return (true);
        }

        if ((event->state & MODIFIERS) == GDK_CONTROL_MASK) {
          float inc = 0.0;
          if (event->direction == GDK_SCROLL_UP) inc = -ZOOM_MULTIPLIER;
          else if (event->direction == GDK_SCROLL_DOWN) inc = ZOOM_MULTIPLIER;
          else return (false);

          pane.FOV *= exp (inc);
          if (pane.FOV < 0.1) pane.FOV = 0.1;
          if (pane.FOV > 2000.0) pane.FOV = 2000.0;

          Window::Main->update();
          return (true); 
        }

        return (false); 
      }





      bool Normal::on_key_press (GdkEventKey* event)
      {
        Slice::Current S (pane);
        if (!S.image) return (false);

        if (!(event->state & MODIFIERS)) {
          if (event->keyval == GDK_Up) {
            move_slice (S, 1.0);
            return (true);
          }
          if (event->keyval == GDK_Down) {
            move_slice (S, -1.0);
            return (true);
          }
          if (event->keyval == GDK_Left) {
            if (S.image->interp->ndim() > 3 && S.channel[3] > 0) { S.channel[3]--; Window::Main->update(); }
          }
          if (event->keyval == GDK_Right) {
            if (S.image->interp->ndim() > 3 && S.channel[3] < (int) S.image->interp->dim(3) - 1) { S.channel[3]++; Window::Main->update(); }
          }
        }
        return (false);
      }






      void Normal::move_slice (Slice::Current& S, float dist)
      {
        Point inc;
        if (S.orientation) {
          const GLdouble* M = pane.get_modelview();
          Point norm (M[2], M[6], M[10]);
          S.image->vox_vector (inc, norm);
          inc = - dist * inc.norm() * norm;
        }
        else {
          MR::Image::Interp& I (*S.image->interp);
          inc.zero();
          inc[S.projection] = dist;
          inc = I.vec_P2R (inc);
        }

        S.focus += inc;
        Window::Main->update();
      }




      void Normal::set_focus (Slice::Current& S, double x, double y)
      {
        Point f = pane.model_to_screen (S.focus);
        f[0] = x; 
        f[1] = pane.height() - y;
        S.focus = pane.screen_to_model (f);
        Window::Main->update();
      }




    }
  }
}


