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

#include <QMenu>

#include "mrview/image.h"
#include "mrview/window.h"

namespace MR {
  namespace Viewer {

    Image::Image (Window& parent, MR::Image::Header* header) :
      QAction (header->name().c_str(), &parent),
      H (*header),
      vox (H),
      interp (H),
      window (parent),
      value_min (NAN),
      value_max (NAN),
      display_min (NAN),
      display_max (NAN)
    {
      assert (header);
      setCheckable (true);
      window.image_group->addAction (this);
      window.image_menu->addAction (this);
      texture2D[0] = texture2D[1] = texture2D[2] = 0;
      slice_position[0] = slice_position[1] = slice_position[2] = 0;
    }

    Image::~Image () { }

    void Image::reset_windowing () 
    { 
      if (isnan (value_min) || isnan (value_max)) {
      }
      display_min = value_min;
      display_max = value_max;
    }



    void Image::render2D (int projection, int slice)
    {
      VAR (projection);
      VAR (slice);
      update_texture2D (projection, slice);

      //int i = slice.interpolate ? GL_LINEAR : GL_NEAREST;
      int i = GL_LINEAR;
      glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, i);
      glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, i);

      int x, y;
      get_axes (projection, x, y);
      size_t xdim = H.dim(x), ydim = H.dim(y);

      Point p, q;
      p[projection] = slice;

      glBegin (GL_QUADS);
      glTexCoord2f (0.0, 0.0); p[x] =    0; p[y] =    0; q = interp.voxel2scanner (p); glVertex3fv (q.get());
      glTexCoord2f (0.0, 1.0); p[x] =    0; p[y] = ydim; q = interp.voxel2scanner (p); glVertex3fv (q.get());
      glTexCoord2f (1.0, 1.0); p[x] = xdim; p[y] = ydim; q = interp.voxel2scanner (p); glVertex3fv (q.get());
      glTexCoord2f (1.0, 0.0); p[x] = xdim; p[y] =    0; q = interp.voxel2scanner (p); glVertex3fv (q.get());
      glEnd();
    }


    inline void Image::update_texture2D (int projection, int slice)
    {
      if (slice_position[projection] == slice && texture2D[projection]) return;
      slice_position[projection] = slice;

      if (!texture2D[projection]) { // allocate:
        glGenTextures (1, &texture2D[projection]);
        assert (texture2D[projection]);
        glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
        glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
      }

      int x, y;
      get_axes (projection, x, y);
      VAR (x);
      VAR (y);
      ssize_t xdim = H.dim(x), ydim = H.dim(y);
      float data [xdim*ydim];

      // copy data:
      vox[projection] = slice;
      for (vox[y] = 0; vox[y] < ydim; ++vox[y])
        for (vox[x] = 0; vox[x] < ydim; ++vox[x])
          data[vox[x]+vox[y]*xdim] = vox.value();

      if (isnan (value_min) || isnan (value_max)) { // reset windowing:
        value_min = INFINITY;
        value_max = -INFINITY;
        for (ssize_t i = 0; i < xdim*ydim; ++i) {
          if (data[i] < value_min) value_min = data[i];
          if (data[i] > value_max) value_max = data[i];
        }
        display_min = value_min;
        display_max = value_max;
      }

      glBindTexture (GL_TEXTURE_2D, texture2D[projection]);
      glTexImage2D (GL_TEXTURE_2D, 0, GL_LUMINANCE32F_ARB, xdim, ydim, 0, GL_LUMINANCE, GL_FLOAT, data);
    }
  }
}



