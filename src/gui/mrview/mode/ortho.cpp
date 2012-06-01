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

#include <QAction>

#include "mrtrix.h"
#include "gui/cursor.h"
#include "math/vector.h"
#include "gui/mrview/mode/ortho.h"

namespace MR
{
  namespace GUI
  {
    namespace MRView
    {
      namespace Mode
      {

        Ortho::Ortho (Window& parent) : 
          Base (parent),
          current_projection (-1) { }

        Ortho::~Ortho () { }




        void Ortho::paint ()
        {
          if (!focus()) reset_view();
          if (!target()) set_target (focus());

          GLint w = glarea()->width()/2;
          GLint h = glarea()->height()/2;
          float fov = FOV() / float(w+h);
          float fovx = w * fov;
          float fovy = h * fov;

          glViewport (w, h, w, h);
          draw_projection (0, fovx, fovy);

          glViewport (0, h, w, h);
          draw_projection (1, fovx, fovy);

          glViewport (0, 0, w, h);
          draw_projection (2, fovx, fovy);

          glViewport (0, 0, glarea()->width(), glarea()->height());
        }








        void Ortho::draw_projection (int proj, float fovx, float fovy)
        {
          // set up modelview matrix:
          const float* Q = image()->interp.image2scanner_matrix();
          float M[16];

          adjust_projection_matrix (M, Q, proj);

          // image slice:
          Point<> voxel (image()->interp.scanner2voxel (focus()));
          int slice = Math::round (voxel[proj]);

          // camera target:
          Point<> F = image()->interp.scanner2voxel (target());
          F[proj] = slice;
          F = image()->interp.voxel2scanner (F);

          // info for projection:
          float depth = image()->interp.dim (proj) * image()->interp.vox (proj);

          // set up projection & modelview matrices:
          glMatrixMode (GL_PROJECTION);
          glLoadIdentity ();
          glOrtho (-fovx, fovx, -fovy, fovy, -depth, depth);

          glMatrixMode (GL_MODELVIEW);
          glLoadIdentity ();
          glMultMatrixf (M);
          glTranslatef (-F[0], -F[1], -F[2]);

          update_modelview_projection_viewport();
          memcpy (gl_viewport[proj], viewport_matrix, 4*sizeof(GLint));
          memcpy (gl_modelview[proj], modelview_matrix, 16*sizeof(GLdouble));
          memcpy (gl_projection[proj], projection_matrix, 16*sizeof(GLdouble));

          // set up OpenGL environment:
          glDisable (GL_BLEND);
          glEnable (GL_TEXTURE_2D);
          glShadeModel (GL_FLAT);
          glDisable (GL_DEPTH_TEST);
          glTexEnvf (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
          glDepthMask (GL_FALSE);
          glColorMask (GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);

          // render image:
          image()->render2D (proj, slice);

          glDisable (GL_TEXTURE_2D);

          draw_focus();

          if (show_orientation_action->isChecked()) {
            glColor4f (1.0, 0.0, 0.0, 1.0);
            switch (proj) {
              case 0:
                renderText ("A", LeftEdge);
                renderText ("S", TopEdge);
                renderText ("P", RightEdge);
                renderText ("I", BottomEdge);
                break;
              case 1:
                renderText ("R", LeftEdge);
                renderText ("S", TopEdge);
                renderText ("L", RightEdge);
                renderText ("I", BottomEdge);
                break;
              case 2:
                renderText ("R", LeftEdge);
                renderText ("A", TopEdge);
                renderText ("L", RightEdge);
                renderText ("P", BottomEdge);
                break;
              default:
                assert (0);
            }
          }
        }



        void Ortho::set_cursor ()
        {
          if (mouse_edge() == (RightEdge | BottomEdge))
            glarea()->setCursor (Cursor::window);
          else if (mouse_edge() & LeftEdge)
            glarea()->setCursor (Cursor::zoom);
          else
            glarea()->setCursor (Cursor::crosshair);
        }





        void Ortho::set_focus (const QPoint& pos) {
          if (current_projection < 0) return;
          Base::set_focus (screen_to_model (pos, gl_viewport[current_projection], gl_modelview[current_projection], gl_projection[current_projection]));
          updateGL();
        }



        void Ortho::adjust_target (const QPoint& dpos) {
          if (current_projection < 0) return;
          Point<> pos = screen_to_model_direction (dpos, gl_viewport[current_projection], gl_modelview[current_projection], gl_projection[current_projection]); 
          set_target (target() - pos);
          updateGL();
        }




        bool Ortho::mouse_click ()
        {
          if (mouse_pos().x() < glarea()->width()/2) 
            if (mouse_pos().y() < glarea()->height()/2) 
              current_projection = 1;
            else 
              current_projection = 2;
          else 
            if (mouse_pos().y() < glarea()->height()/2)
              current_projection = 0;
            else 
              current_projection = -1;


          if (mouse_modifiers() == Qt::NoModifier) {

            if (mouse_buttons() == Qt::LeftButton) {
              glarea()->setCursor (Cursor::crosshair);
              set_focus (mouse_pos());
              return true;
            }

            if (mouse_buttons() == Qt::RightButton) {
              if (!mouse_edge() && current_projection >= 0) {
                glarea()->setCursor (Cursor::pan_crosshair);
                return true;
              }
            }

          }
          return false;
        }





        bool Ortho::mouse_move ()
        {
          if (mouse_buttons() == Qt::NoButton) {
            set_cursor();
            return false;
          }


          if (mouse_modifiers() == Qt::NoModifier) {

            if (mouse_buttons() == Qt::LeftButton) {
              set_focus (currentPos);
              return true;
            }

            if (mouse_buttons() == Qt::RightButton) {

              if (mouse_edge() == (RightEdge | BottomEdge)) {
                image()->adjust_windowing (mouse_dpos());
                updateGL();
                return true;
              }

              if (mouse_edge() & LeftEdge) {
                change_FOV_fine (mouse_dpos().y());
                updateGL();
                return true;
              }

              adjust_target (mouse_dpos());
              return true;
            }
          }

          return false;
        }




        bool Ortho::mouse_release ()
        {
          set_cursor();
          current_projection = -1;
          return true;
        }




        bool Ortho::mouse_wheel (float delta, Qt::Orientation orientation)
        {
          if (orientation == Qt::Vertical) {

            if (mouse_modifiers() == Qt::ControlModifier) {
              change_FOV_scroll (-delta);
              updateGL();
              return true;
            }
          }

          return false; 
        }





        void Ortho::reset_view ()
        {
          if (!image()) return;
          float dim[] = {
            image()->header().dim (0)* image()->header().vox (0),
            image()->header().dim (1)* image()->header().vox (1),
            image()->header().dim (2)* image()->header().vox (2)
          };

          Point<> p (image()->header().dim (0) /2.0, image()->header().dim (1) /2.0, image()->header().dim (2) /2.0);
          Base::set_focus (image()->interp.voxel2scanner (p));

          set_FOV (std::max (dim[0], std::max (dim[1], dim[2])));

          set_target (Point<>());
        }



        void Ortho::reset ()
        {
          reset_view();
          updateGL();
        }


      }
    }
  }
}



