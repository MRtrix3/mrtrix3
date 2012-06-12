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

#include "math/vector.h"
#include "gui/mrview/mode/volume.h"

namespace MR
{
  namespace GUI
  {
    namespace MRView
    {
      namespace Mode
      {

        Volume::Volume (Window& parent) : Mode3D (parent) { }

        Volume::~Volume () { }

        void Volume::paint ()
        {
          if (!focus()) reset_view();
          if (!target()) set_target (focus());

          // info for projection:
          int w = glarea()->width(), h = glarea()->height();
          float fov = FOV() / (float) (w+h);
          float depth = std::max (image()->interp.dim(0)*image()->interp.vox(0),
              std::max (image()->interp.dim(1)*image()->interp.vox(1),
                image()->interp.dim(2)*image()->interp.vox(2)));



          // set up projection & modelview matrices:
          glMatrixMode (GL_PROJECTION);
          glLoadIdentity ();
          glOrtho (-w*fov, w*fov, -h*fov, h*fov, -depth, depth);

          glMatrixMode (GL_MODELVIEW);
          glLoadIdentity ();

          Math::Quaternion<float> Q = orientation();
          if (!Q) {
            Q = Math::Quaternion<float> (1.0, 0.0, 0.0, 0.0);
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


          glTranslatef (-target() [0], -target() [1], -target() [2]);
          update_modelview_projection_viewport();

          // set up OpenGL environment:
          glEnable (GL_TEXTURE_3D);
          glShadeModel (GL_FLAT);
          glDisable (GL_DEPTH_TEST);
          glTexEnvf (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
          glDepthMask (GL_FALSE);
          glColorMask (GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);

          glEnable (GL_BLEND);
          glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

          // render image:
          image()->render3D_pre (*this);
          float increment = std::min (image()->interp.vox(0), std::min (image()->interp.vox(1), image()->interp.vox(2)));
          for (float offset = -depth; offset <= depth; offset += increment)
            image()->render3D_slice (*this, offset);
          image()->render3D_post();

          glDisable (GL_TEXTURE_3D);
          glDisable (GL_BLEND);

          draw_focus();

          draw_orientation_labels();
        }




      }
    }
  }
}



