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
#include "gui/mrview/mode/mode2d.h"

namespace MR
{
  namespace GUI
  {
    namespace MRView
    {
      namespace Mode
      {

        Mode2D::Mode2D (Window& parent, int flags) : Base (parent, flags) { }

        Mode2D::~Mode2D () { }

        void Mode2D::paint ()
        {
          if (!focus()) reset_view();
          if (!target()) set_target (focus());

          // set up modelview matrix:
          const float* Q = image()->interp.image2scanner_matrix();
          float M[16];

          adjust_projection_matrix (M, Q);

          // image slice:
          Point<> voxel (image()->interp.scanner2voxel (focus()));
          int slice = Math::round (voxel[plane()]);

          // camera target:
          Point<> F = image()->interp.scanner2voxel (target());
          F[plane()] = slice;
          F = image()->interp.voxel2scanner (F);

          // info for projection:
          int w = glarea()->width(), h = glarea()->height();
          float fov = FOV() / (float) (w+h);
          float depth = image()->interp.dim (plane()) * image()->interp.vox (plane());

          // set up projection & modelview matrices:
          glMatrixMode (GL_PROJECTION);
          glLoadIdentity ();
          glOrtho (-w*fov, w*fov, -h*fov, h*fov, -depth, depth);

          glMatrixMode (GL_MODELVIEW);
          glLoadIdentity ();
          glMultMatrixf (M);
          glTranslatef (-F[0], -F[1], -F[2]);

          projection.update();

          // set up OpenGL environment:
          glDisable (GL_BLEND);
          glEnable (GL_TEXTURE_2D);
          glShadeModel (GL_FLAT);
          glDisable (GL_DEPTH_TEST);
          glTexEnvf (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
          glDepthMask (GL_FALSE);
          glColorMask (GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);

          // render image:
          image()->render2D (plane(), slice);

          glDisable (GL_TEXTURE_2D);

          if (window.show_crosshairs()) 
            projection.render_crosshairs (focus());

          if (window.show_orientation_labels()) {
            glColor4f (1.0, 0.0, 0.0, 1.0);
            switch (plane()) {
              case 0:
                projection.render_text ("A", LeftEdge);
                projection.render_text ("S", TopEdge);
                projection.render_text ("P", RightEdge);
                projection.render_text ("I", BottomEdge);
                break;
              case 1:
                projection.render_text ("R", LeftEdge);
                projection.render_text ("S", TopEdge);
                projection.render_text ("L", RightEdge);
                projection.render_text ("I", BottomEdge);
                break;
              case 2:
                projection.render_text ("R", LeftEdge);
                projection.render_text ("A", TopEdge);
                projection.render_text ("L", RightEdge);
                projection.render_text ("P", BottomEdge);
                break;
              default:
                assert (0);
            }
          }
        }




        void Mode2D::reset_view ()
        {
          if (!image()) return;
          float dim[] = {
            image()->header().dim (0)* image()->header().vox (0),
            image()->header().dim (1)* image()->header().vox (1),
            image()->header().dim (2)* image()->header().vox (2)
          };
          if (dim[0] < dim[1] && dim[0] < dim[2])
            set_plane (0);
          else if (dim[1] < dim[0] && dim[1] < dim[2])
            set_plane (1);
          else
            set_plane (2);

          Point<> p (image()->header().dim (0) /2.0, image()->header().dim (1) /2.0, image()->header().dim (2) /2.0);
          set_focus (image()->interp.voxel2scanner (p));

          int x, y;
          image()->get_axes (plane(), x, y);
          set_FOV (std::max (dim[x], dim[y]));

          set_target (Point<>());
        }



        void Mode2D::slice_move_event (int x)
        {
          if (!image()) return;
          move_in_out (x * image()->header().vox (plane()));
          updateGL();
        }






        void Mode2D::reset_event ()
        {
          reset_view();
          updateGL();
        }



        void Mode2D::set_focus_event ()
        {
          set_focus (projection.screen_to_model (window.mouse_position(), focus()));
          updateGL();
        }


        void Mode2D::contrast_event ()
        {
          image()->adjust_windowing (window.mouse_displacement());
          window.scaling_updated();
          updateGL();
        }

        void Mode2D::pan_event ()
        {
          set_target (target() - projection.screen_to_model_direction (window.mouse_displacement(), 0.0f));
          updateGL();
        }


        void Mode2D::panthrough_event ()
        {
          move_in_out_FOV (window.mouse_displacement().y());
          updateGL();
        }


      }
    }
  }
}

