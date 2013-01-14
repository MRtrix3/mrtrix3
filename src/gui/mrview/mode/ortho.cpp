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
          current_plane (-1) { 
            projections.push_back (projection);
            projections.push_back (projection);
            projections.push_back (projection);
          }

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
          draw_plane (0, fovx, fovy);

          glViewport (0, h, w, h);
          draw_plane (1, fovx, fovy);

          glViewport (0, 0, w, h);
          draw_plane (2, fovx, fovy);

          glViewport (0, 0, glarea()->width(), glarea()->height());
          glMatrixMode (GL_PROJECTION);
          glLoadIdentity ();
          glOrtho (0, glarea()->width(), 0, glarea()->height(), -1.0, 1.0);
          glMatrixMode (GL_MODELVIEW);
          glLoadIdentity ();
          glDisable (GL_DEPTH_TEST);

          glColor4f (0.1, 0.1, 0.1, 1.0);
          glLineWidth (2.0);

          glBegin (GL_LINES);
          glVertex2f (glarea()->width()/2, 0.0);
          glVertex2f (glarea()->width()/2, glarea()->height());
          glVertex2f (0.0, glarea()->height()/2);
          glVertex2f (glarea()->width(), glarea()->height()/2);
          glEnd();

          glEnable (GL_DEPTH_TEST);
        }





        void Ortho::draw_plane (int axis, float fovx, float fovy)
        {
          // set up modelview matrix:
          const float* Q = image()->interp.image2scanner_matrix();
          float M[16];

          adjust_projection_matrix (M, Q, axis);

          // image slice:
          Point<> voxel (image()->interp.scanner2voxel (focus()));
          int slice = Math::round (voxel[axis]);

          // camera target:
          Point<> F = image()->interp.scanner2voxel (target());
          F[axis] = slice;
          F = image()->interp.voxel2scanner (F);

          // info for projection:
          float depth = image()->interp.dim (axis) * image()->interp.vox (axis);

          // set up projection & modelview matrices:
          glMatrixMode (GL_PROJECTION);
          glLoadIdentity ();
          glOrtho (-fovx, fovx, -fovy, fovy, -depth, depth);

          glMatrixMode (GL_MODELVIEW);
          glLoadIdentity ();
          glMultMatrixf (M);
          glTranslatef (-F[0], -F[1], -F[2]);

          projections[axis].update();

          // set up OpenGL environment:
          glDisable (GL_BLEND);
          glEnable (GL_TEXTURE_2D);
          glShadeModel (GL_FLAT);
          glDisable (GL_DEPTH_TEST);
          glTexEnvf (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
          glDepthMask (GL_FALSE);
          glColorMask (GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);

          // render image:
          image()->render2D (axis, slice);

          glDisable (GL_TEXTURE_2D);

          render_tools2D (projections[axis]);

          if (window.show_crosshairs()) 
            projections[axis].render_crosshairs (focus());

          if (window.show_orientation_labels()) {
            glColor4f (1.0, 0.0, 0.0, 1.0);
            switch (axis) {
              case 0:
                projections[axis].render_text ("A", LeftEdge);
                projections[axis].render_text ("S", TopEdge);
                projections[axis].render_text ("P", RightEdge);
                projections[axis].render_text ("I", BottomEdge);
                break;
              case 1:
                projections[axis].render_text ("R", LeftEdge);
                projections[axis].render_text ("S", TopEdge);
                projections[axis].render_text ("L", RightEdge);
                projections[axis].render_text ("I", BottomEdge);
                break;
              case 2:
                projections[axis].render_text ("R", LeftEdge);
                projections[axis].render_text ("A", TopEdge);
                projections[axis].render_text ("L", RightEdge);
                projections[axis].render_text ("P", BottomEdge);
                break;
              default:
                assert (0);
            }
          }

        }




        void Ortho::reset_event ()
        {
          reset_view();
          updateGL();
        }



        void Ortho::mouse_press_event ()
        {
          if (window.mouse_position().x() < glarea()->width()/2) 
            if (window.mouse_position().y() >= glarea()->height()/2) 
              current_plane = 1;
            else 
              current_plane = 2;
          else 
            if (window.mouse_position().y() >= glarea()->height()/2)
              current_plane = 0;
            else 
              current_plane = -1;
        }




        void Ortho::slice_move_event (int x) 
        { 
          if (current_plane < 0) return;
          set_focus (focus() + move_in_out_displacement (x * image()->header().vox (current_plane),projections[current_plane]));
          updateGL();
        } 


        void Ortho::set_focus_event ()
        {
          if (current_plane < 0) 
            return;
          Base::set_focus (projections[current_plane].screen_to_model (window.mouse_position(), focus()));
          updateGL();
        }


        void Ortho::contrast_event ()
        {
          image()->adjust_windowing (window.mouse_displacement());
          window.on_scaling_changed();
          updateGL();
        }






        void Ortho::pan_event () 
        {
          if (current_plane < 0) return;
          set_target (target() - projections[current_plane].screen_to_model_direction (window.mouse_displacement(), target()));
          updateGL();
        }


        void Ortho::panthrough_event () 
        { 
          if (current_plane < 0) return;
          set_focus (focus() + move_in_out_displacement (window.mouse_displacement().y(), projections[current_plane]));
          updateGL();
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




      }
    }
  }
}



