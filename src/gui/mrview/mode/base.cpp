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

#include "file/config.h"
#include "gui/opengl/gl.h"
#include "gui/mrview/mode/base.h"

namespace MR
{
  namespace GUI
  {
    namespace MRView
    {
      namespace Mode
      {

        Base::Base (Window& parent, int flags) :
          window (parent),
          mouse_actions (flags),
          painting (false)
        {
          font_.setPointSize (MR::File::Config::get_int ("FontSize", 10));
          modelview_matrix[0] = NAN;
        }


        Base::~Base ()
        {
          glarea()->setCursor (Cursor::crosshair);
        }

        void Base::paintGL ()
        {
          painting = true;

          glViewport (0, 0, glarea()->width(), glarea()->height());
          update_modelview_projection_viewport();

          glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
          if (!image()) {
            renderText (10, 10, "No image loaded");
            goto done_painting;
          }

          {
            paint();

            glDisable (GL_MULTISAMPLE);
            update_modelview_projection_viewport();
            glColor4f (1.0, 1.0, 0.0, 1.0);

            if (window.show_voxel_info()) {
              Point<> voxel (image()->interp.scanner2voxel (focus()));
              Image::VoxelType& imvox (image()->voxel());
              ssize_t vox [] = { Math::round<int> (voxel[0]), Math::round<int> (voxel[1]), Math::round<int> (voxel[2]) };

              std::string vox_str = printf ("voxel: [ %d %d %d ", vox[0], vox[1], vox[2]);
              for (size_t n = 3; n < imvox.ndim(); ++n) 
                vox_str += str(imvox[n]) + " ";
              vox_str += "]";

              renderText (printf ("position: [ %.4g %.4g %.4g ] mm", focus() [0], focus() [1], focus() [2]), LeftEdge | BottomEdge);
              renderText (vox_str, LeftEdge | BottomEdge, 1);
              std::string value;
              if (vox[0] >= 0 && vox[0] < imvox.dim (0) &&
                  vox[1] >= 0 && vox[1] < imvox.dim (1) &&
                  vox[2] >= 0 && vox[2] < imvox.dim (2)) {
                imvox[0] = vox[0];
                imvox[1] = vox[1];
                imvox[2] = vox[2];
                cfloat val = imvox.value();
                value = "value: " + str (val);
              }
              else value = "value: ?";
              renderText (value, LeftEdge | BottomEdge, 2);
            }

            if (window.show_comments()) {
              for (size_t i = 0; i < image()->header().comments().size(); ++i)
                renderText (image()->header().comments() [i], LeftEdge | TopEdge, i);
            }
          }

done_painting:
          painting = false;
        }


        void Base::paint () { }
        void Base::reset_event () { }
        void Base::mouse_press_event () { }
        void Base::mouse_release_event () { }
        void Base::slice_move_event (int x) { }
        void Base::set_focus_event () { }
        void Base::contrast_event () { }
        void Base::pan_event () { }
        void Base::panthrough_event () { }
        void Base::tilt_event () { }
        void Base::rotate_event () { }

        void Base::draw_focus () const
        {
          // draw focus:
          if (window.show_crosshairs()) {
            glPushAttrib (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            glDepthMask (GL_FALSE);
            Point<> F = model_to_screen (focus());
            F[0] -= viewport_matrix[0];
            F[1] -= viewport_matrix[1];

            glMatrixMode (GL_PROJECTION);
            glPushMatrix ();
            glLoadIdentity ();
            glOrtho (0, width(), 0, height(), -1.0, 1.0);
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
            glVertex2f (width(), F[1]);
            glVertex2f (F[0], 0.0);
            glVertex2f (F[0], height());
            glEnd ();

            glDisable (GL_BLEND);
            glPopMatrix ();
            glMatrixMode (GL_PROJECTION);
            glPopMatrix ();
            glMatrixMode (GL_MODELVIEW);
            glPopAttrib();
          }
        }

        void Base::adjust_projection_matrix (float* M, const float* Q, int proj) const
        {
          M[3] = M[7] = M[11] = M[12] = M[13] = M[14] = 0.0;
          M[15] = 1.0;
          if (proj == 0) { // sagittal
            for (size_t n = 0; n < 3; n++) {
              M[4*n]   = -Q[4*n+1];  // x: -y
              M[4*n+1] =  Q[4*n+2];  // y: z
              M[4*n+2] =  Q[4*n];    // z: x
            }
          }
          else if (proj == 1) { // coronal
            for (size_t n = 0; n < 3; n++) {
              M[4*n]   = -Q[4*n];    // x: -x
              M[4*n+1] =  Q[4*n+2];  // y: z
              M[4*n+2] = -Q[4*n+1];  // z: -y
            }
          }
          else { // axial
            for (size_t n = 0; n < 3; n++) {
              M[4*n]   = -Q[4*n];    // x: -x
              M[4*n+1] =  Q[4*n+1];  // y: y
              M[4*n+2] =  Q[4*n+2];  // z: z
            }
          }
        }

      }
    }
  }
}

#undef MODE


