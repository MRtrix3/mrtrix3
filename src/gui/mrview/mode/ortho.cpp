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
          current_projection (-1) { 
            transforms.push_back (transform);
            transforms.push_back (transform);
            transforms.push_back (transform);
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

          transforms[proj].update();

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

          if (window.show_crosshairs()) 
            transforms[proj].render_crosshairs (focus());

          if (window.show_orientation_labels()) {
            glColor4f (1.0, 0.0, 0.0, 1.0);
            switch (proj) {
              case 0:
                transforms[proj].render_text ("A", LeftEdge);
                transforms[proj].render_text ("S", TopEdge);
                transforms[proj].render_text ("P", RightEdge);
                transforms[proj].render_text ("I", BottomEdge);
                break;
              case 1:
                transforms[proj].render_text ("R", LeftEdge);
                transforms[proj].render_text ("S", TopEdge);
                transforms[proj].render_text ("L", RightEdge);
                transforms[proj].render_text ("I", BottomEdge);
                break;
              case 2:
                transforms[proj].render_text ("R", LeftEdge);
                transforms[proj].render_text ("A", TopEdge);
                transforms[proj].render_text ("L", RightEdge);
                transforms[proj].render_text ("P", BottomEdge);
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
              current_projection = 1;
            else 
              current_projection = 2;
          else 
            if (window.mouse_position().y() >= glarea()->height()/2)
              current_projection = 0;
            else 
              current_projection = -1;
        }




        void Ortho::slice_move_event (int x) 
        { 
          if (current_projection < 0) return;
          set_focus (focus() + move_in_out_displacement (x * image()->header().vox (current_projection),transforms[current_projection]));
          updateGL();
        } 


        void Ortho::set_focus_event ()
        {
          if (current_projection < 0) 
            return;
          Base::set_focus (transforms[current_projection].screen_to_model (window.mouse_position(), focus()));
          updateGL();
        }


        void Ortho::contrast_event ()
        {
          image()->adjust_windowing (window.mouse_displacement());
          window.scaling_updated();
          updateGL();
        }






        void Ortho::pan_event () 
        {
          if (current_projection < 0) return;
          set_target (target() - transforms[current_projection].screen_to_model_direction (window.mouse_displacement()));
          updateGL();
        }


        void Ortho::panthrough_event () 
        { 
          if (current_projection < 0) return;
          set_focus (focus() + move_in_out_displacement (window.mouse_displacement().y(), transforms[current_projection]));
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



