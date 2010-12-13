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

#include "mrview/mode/mode3d.h"
#include "math/vector.h"

#define ROTATION_INC 0.001

namespace MR {
  namespace Viewer {
    namespace Mode {

      Mode3D::Mode3D (Window& parent) : Base (parent) { }
      Mode3D::~Mode3D () { }

      void Mode3D::paint () 
      { 
        if (!target()) set_target (focus());

        // camera target:
        //Point<> F = target();

        // info for projection:
        int w = glarea()->width(), h = glarea()->height();
        float fov = FOV() / (float) (w+h);
        float depth = 100.0;

        // set up projection & modelview matrices:
        glMatrixMode (GL_PROJECTION);
        glLoadIdentity ();
        glOrtho (-w*fov, w*fov, -h*fov, h*fov, -depth, depth);

        glMatrixMode (GL_MODELVIEW);
        glLoadIdentity ();

        Math::Quaternion Q = orientation();
        if (!Q) {
          Q = Math::Quaternion (1.0, 0.0, 0.0, 0.0);
          set_orientation (Q);
        }

        float M[9];
        Q.to_matrix (M);
        float T [] = {
          M[0], M[1], M[2], 0.0,
          M[3], M[4], M[5], 0.0,
          M[6], M[7], M[8], 0.0,
          0.0, 0.0, 0.0, 1.0
        };
        float S[16];
        adjust_projection_matrix (S, T);
        glMultMatrixf (S);

        glTranslatef (-target()[0], -target()[1], -target()[2]);
        get_modelview_projection_viewport();

        // set up OpenGL environment:
        glDisable (GL_BLEND);
        glEnable (GL_TEXTURE_3D);
        glShadeModel (GL_FLAT);
        glDisable (GL_DEPTH_TEST);
        glTexEnvf (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
        glDepthMask (GL_FALSE);
        glColorMask (GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);

        // render image:
        DEBUG_OPENGL;
        image()->render3D (*this);
        DEBUG_OPENGL;

        glDisable (GL_TEXTURE_3D);

        draw_focus();
      }





      bool Mode3D::mouse_click ()
      { 
        if (mouse_modifiers() == Qt::NoModifier) {

          if (mouse_buttons() == Qt::LeftButton) {
            if (!mouse_edge()) {
              set_focus (screen_to_model (mouse_pos()));
              updateGL();
              return true;
            }
          }

          else if (mouse_buttons() == Qt::RightButton) 
            glarea()->setCursor (Cursor::pan_crosshair);
        }

        return false; 
      }





      bool Mode3D::mouse_move () 
      {
        if (mouse_buttons() == Qt::NoButton) {
          if (mouse_edge() == ( RightEdge | BottomEdge ))
            glarea()->setCursor (Cursor::window);
          else if (mouse_edge() & RightEdge) 
            glarea()->setCursor (Cursor::forward_backward);
          else if (mouse_edge() & LeftEdge) 
            glarea()->setCursor (Cursor::zoom);
          else if (mouse_edge() & TopEdge) 
            glarea()->setCursor (Cursor::pan_crosshair);
          else
            glarea()->setCursor (Cursor::crosshair);
          return false;
        }

        if (mouse_modifiers() == Qt::NoModifier) {

          if (mouse_buttons() == Qt::LeftButton) {

            if (mouse_edge() == ( RightEdge | BottomEdge) ) {
              image()->adjust_windowing (mouse_dpos_static());
              updateGL();
              return true;
            }

            if (mouse_edge() & RightEdge) {
              move_in_out (-0.001*mouse_dpos().y()*FOV());
              updateGL();
              return true;
            }

            if (mouse_edge() & LeftEdge) {
              change_FOV_fine (mouse_dpos().y());
              updateGL();
              return true;
            }

            set_focus (screen_to_model());
            updateGL();
            return true;
          }

          if (mouse_buttons() == Qt::RightButton) {
            set_target (target() - screen_to_model_direction (Point<> (mouse_dpos().x(), mouse_dpos().y(), 0.0)));
            updateGL();
            return true;
          }

          if (mouse_buttons() == Qt::MiddleButton) {
            if (mouse_dpos().x() == 0 && mouse_dpos().y() == 0) 
              return true;
            Point<> x = screen_to_model_direction (Point<> (mouse_dpos().x(), mouse_dpos().y(), 0.0));
            Point<> z = screen_to_model_direction (Point<> (0.0, 0.0, 1.0));
            Point<> v;
            Math::cross (v.get(), x.get(), z.get());
            float angle = ROTATION_INC * Math::sqrt (float (Math::pow2(mouse_dpos().x()) + Math::pow2(mouse_dpos().y())));
            v.normalise();
            if (angle > M_PI_2) angle = M_PI_2;

            Math::Quaternion q = Math::Quaternion (angle, v.get()) * orientation();
            q.normalise();
            set_orientation (q);
            updateGL();
            return true;
          }

        }
        return false; 
      }






      bool Mode3D::mouse_release () 
      { 
        if (mouse_edge() == ( RightEdge | BottomEdge)) 
          glarea()->setCursor (Cursor::window);
        else if (mouse_edge() & RightEdge) 
          glarea()->setCursor (Cursor::forward_backward);
        else if (mouse_edge() & LeftEdge) 
          glarea()->setCursor (Cursor::zoom);
        else 
          glarea()->setCursor (Cursor::crosshair);
        return true; 
      }


      bool Mode3D::mouse_wheel (float delta, Qt::Orientation orientation) { return false; }

    }
  }
}


