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

#include "gui/opengl/lighting.h"
#include "math/vector.h"
#include "gui/mrview/mode/volume.h"
#include "gui/mrview/mode/volume_extra_controls.h"
#include "gui/mrview/tool/base.h"
#include "gui/mrview/adjust_button.h"
#include "gui/dialog/lighting.h"

namespace MR
{
  namespace GUI
  {
    namespace MRView
    {
      namespace Mode
      {

        Volume::Volume (Window& parent) : 
          Slice (parent),
          extra_controls (NULL) { 
          }

        Volume::~Volume () 
        { 
          delete extra_controls;
        }

        void Volume::paint (Projection& projection)
        {
          // info for projection:
          int w = glarea()->width(), h = glarea()->height();
          float fov = FOV() / (float) (w+h);

          float depth = std::max (image()->interp.dim(0)*image()->interp.vox(0),
              std::max (image()->interp.dim(1)*image()->interp.vox(1),
                image()->interp.dim(2)*image()->interp.vox(2)));


          Math::Versor<float> Q = orientation();
          if (!Q) {
            Q = Math::Versor<float> (1.0, 0.0, 0.0, 0.0);
            set_orientation (Q);
          }

          // set up projection & modelview matrices:
          GL::mat4 P = GL::ortho (-w*fov, w*fov, -h*fov, h*fov, -depth, depth);
          GL::mat4 MV = adjust_projection_matrix (Q) * GL::translate  (-target()[0], -target()[1], -target()[2]);
          projection.set (MV, P);

          // find min/max depth of texture:
          Point<> z = projection.screen_normal();
          z.normalise();
          float d;
          float mindepth = std::numeric_limits<float>::infinity();
          float maxdepth = -std::numeric_limits<float>::infinity();

          Point<> top (
              image()->interp.dim(0)-0.5, 
              image()->interp.dim(1)-0.5, 
              image()->interp.dim(2)-0.5);

          d = z.dot (image()->interp.voxel2scanner(Point<> (-0.5, -0.5, -0.5)));
          if (d < mindepth) mindepth = d;
          if (d > maxdepth) maxdepth = d;

          d = z.dot (image()->interp.voxel2scanner(Point<> (top[0], -0.5, -0.5)));
          if (d < mindepth) mindepth = d;
          if (d > maxdepth) maxdepth = d;

          d = z.dot (image()->interp.voxel2scanner(Point<> (-0.5, top[1], -0.5)));
          if (d < mindepth) mindepth = d;
          if (d > maxdepth) maxdepth = d;

          d = z.dot (image()->interp.voxel2scanner(Point<> (-0.5, -0.5, top[2])));
          if (d < mindepth) mindepth = d;
          if (d > maxdepth) maxdepth = d;

          d = z.dot (image()->interp.voxel2scanner(Point<> (top[0], top[1], -0.5)));
          if (d < mindepth) mindepth = d;
          if (d > maxdepth) maxdepth = d;

          d = z.dot (image()->interp.voxel2scanner(Point<> (top[0], -0.5, top[2])));
          if (d < mindepth) mindepth = d;
          if (d > maxdepth) maxdepth = d;

          d = z.dot (image()->interp.voxel2scanner(Point<> (-0.5, top[1], top[2])));
          if (d < mindepth) mindepth = d;
          if (d > maxdepth) maxdepth = d;

          d = z.dot (image()->interp.voxel2scanner(Point<> (top[0], top[1], top[2])));
          if (d < mindepth) mindepth = d;
          if (d > maxdepth) maxdepth = d;

          d = z.dot (focus());
          mindepth -= d;
          maxdepth -= d;


          // set up OpenGL environment:
          glEnable (GL_DEPTH_TEST);

          glDepthMask (GL_FALSE);
          glDisable (GL_BLEND);

          image()->update_texture3D();

          if (isnan (image()->transparent_intensity) ||
              isnan (image()->opaque_intensity)) {
            float range = image()->intensity_max() - image()->intensity_min();
            image()->transparent_intensity = image()->intensity_min() + 0.025 * range;
            image()->opaque_intensity = image()->intensity_min() + 0.475 * range;
            image()->alpha = 1.0;
          }

          image()->set_use_transparency (true);

          glEnable (GL_BLEND);
          glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
          glBlendEquation (GL_FUNC_ADD);

          // render image:
          image()->render3D_pre (projection, projection.depth_of (focus()));
          float increment = std::min (image()->interp.vox(0), std::min (image()->interp.vox(1), image()->interp.vox(2)));
          for (float offset = maxdepth; offset >= mindepth; offset -= increment)
            image()->render3D_slice (offset);
          image()->render3D_post();

          glDisable (GL_TEXTURE_3D);
          glDisable (GL_BLEND);

          if (window.show_crosshairs()) 
            projection.render_crosshairs (focus());

          if (window.show_orientation_labels())
            projection.draw_orientation_labels();
        }




        Tool::Dock* Volume::get_extra_controls () 
        {
          if (!extra_controls) {
            extra_controls = Tool::create<VolumeExtraControls> ("Controls for volume mode", window);
            register_extra_controls (extra_controls);
          }
          return extra_controls;
        }





      }
    }
  }
}



