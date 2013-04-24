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
#include "gui/mrview/mode/slice.h"

namespace MR
{
  namespace GUI
  {
    namespace MRView
    {
      namespace Mode
      {

        Slice::Slice (Window& parent) :
          Base (parent, FocusContrast | MoveTarget | TiltRotate) {
            using namespace App;
            Options opt = get_options ("view");
            if (opt.size()) {
              FAIL ("TODO: apply view option");
            }

          }


        Slice::~Slice () { }


        void Slice::paint (Projection& with_projection)
        {
          // set up OpenGL environment:
          glDisable (GL_BLEND);
          glEnable (GL_TEXTURE_3D);
          glDisable (GL_DEPTH_TEST);
          glTexEnvf (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
          glDepthMask (GL_FALSE);
          glColorMask (GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);

          draw_plane (plane(), with_projection);
        }


        void Slice::draw_plane (int axis, Projection& with_projection)
        {
          // info for projection:
          float fov = FOV() / (float) (with_projection.width()+with_projection.height());
          float depth = std::max (std::max (image()->header().dim(0), image()->header().dim(1)), image()->header().dim(2));

          // set up projection & modelview matrices:
          GL::mat4 P = GL::ortho (
              -with_projection.width()*fov, with_projection.width()*fov,
              -with_projection.height()*fov, with_projection.height()*fov,
              -depth, depth);
          GL::mat4 M = snap_to_image() ? GL::mat4 (image()->interp.image2scanner_matrix()) : GL::mat4 (orientation());
          GL::mat4 MV = adjust_projection_matrix (M, axis) * GL::translate (-target());
          with_projection.set (MV, P);

          // render image:
          DEBUG_OPENGL;
          if (snap_to_image())
            image()->render2D (with_projection, axis, slice(axis));
          else
            image()->render3D (with_projection, with_projection.depth_of (focus()));
          DEBUG_OPENGL;

          glDisable (GL_TEXTURE_3D);

          render_tools2D (with_projection);

          if (window.show_crosshairs())
            with_projection.render_crosshairs (focus());
          if (window.show_orientation_labels())
            with_projection.draw_orientation_labels();
        }






        using namespace App;
        const App::OptionGroup Slice::options = OptionGroup ("single-slice mode")
          + Option ("view", "specify initial angle of view")
          + Argument ("azimuth").type_float(-M_PI, 0.0, M_PI)
          + Argument ("elevation").type_float(0.0, 0.0, M_PI);

      }
    }
  }
}


