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
      QAction (shorten(header->name(), 20, 0).c_str(), &parent),
      H (*header),
      vox (H),
      interp (vox),
      window (parent),
      texture3D (0),
      interpolation (GL_LINEAR),
      value_min (NAN),
      value_max (NAN),
      display_midpoint (NAN),
      display_range (NAN),
      position (vox.ndim())
    {
      assert (header);
      setCheckable (true);
      setToolTip (header->name().c_str());
      setStatusTip (header->name().c_str());
      window.image_group->addAction (this);
      window.image_menu->addAction (this);
      texture2D[0] = texture2D[1] = texture2D[2] = 0;
      position[0] = position[1] = position[2] = std::numeric_limits<ssize_t>::min();
      set_colourmap (0, false);
    }

    Image::~Image () { }



    void Image::render2D (Shader& shader, int projection, int slice)
    {
      update_texture2D (projection, slice);

      glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, interpolation);
      glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, interpolation);

      int x, y;
      get_axes (projection, x, y);
      float xdim = H.dim(x)-0.5, ydim = H.dim(y)-0.5;

      Point<> p, q;
      p[projection] = slice;

      shader.start();

      shader.get_uniform ("offset") = display_midpoint - 0.5f * display_range;
      shader.get_uniform ("scale") = 1.0f / display_range;

      glBegin (GL_QUADS);
      glTexCoord2f (0.0, 0.0); p[x] = -0.5; p[y] = -0.5; q = interp.voxel2scanner (p); glVertex3fv (q.get());
      glTexCoord2f (0.0, 1.0); p[x] = -0.5; p[y] = ydim; q = interp.voxel2scanner (p); glVertex3fv (q.get());
      glTexCoord2f (1.0, 1.0); p[x] = xdim; p[y] = ydim; q = interp.voxel2scanner (p); glVertex3fv (q.get());
      glTexCoord2f (1.0, 0.0); p[x] = xdim; p[y] = -0.5; q = interp.voxel2scanner (p); glVertex3fv (q.get());
      glEnd();
      shader.stop();
    }





    void Image::render3D (Shader& shader, const Math::Quaternion& view, const Point<>& focus)
    {
      update_texture3D ();

      glTexParameteri (GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, interpolation);
      glTexParameteri (GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, interpolation);

      /*
      int x, y;
      get_axes (projection, x, y);
      float xdim = H.dim(x)-0.5, ydim = H.dim(y)-0.5;

      Point<> p, q;
      p[projection] = slice;
      */

      shader.start();

      shader.get_uniform ("offset") = display_midpoint - 0.5f * display_range;
      shader.get_uniform ("scale") = 1.0f / display_range;

      glBegin (GL_QUADS);
      glTexCoord3f (0.0, 0.0, 0.5);// p[x] = -0.5; p[y] = -0.5; q = interp.voxel2scanner (p); glVertex3fv (q.get());
      glTexCoord3f (0.0, 1.0, 0.5);// p[x] = -0.5; p[y] = ydim; q = interp.voxel2scanner (p); glVertex3fv (q.get());
      glTexCoord3f (1.0, 1.0, 0.5);// p[x] = xdim; p[y] = ydim; q = interp.voxel2scanner (p); glVertex3fv (q.get());
      glTexCoord3f (1.0, 0.0, 0.5);// p[x] = xdim; p[y] = -0.5; q = interp.voxel2scanner (p); glVertex3fv (q.get());
      glEnd();
      shader.stop();
    }




    inline void Image::update_texture2D (int projection, int slice)
    {
      if (!texture2D[projection]) { // allocate:
        glGenTextures (1, &texture2D[projection]);
        assert (texture2D[projection]);
        glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
        glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
      }
      glBindTexture (GL_TEXTURE_2D, texture2D[projection]);

      if (position[projection] == slice && volume_unchanged()) 
        return;

      position[projection] = slice;

      int x, y;
      get_axes (projection, x, y);
      ssize_t xdim = H.dim(x), ydim = H.dim(y);
      float data [xdim*ydim];

      if (position[projection] < 0 || position[projection] >= H.dim(projection)) {
        memset (data, 0, xdim*ydim*sizeof(float));
      }
      else {
        // copy data:
        vox[projection] = slice;
        value_min = INFINITY;
        value_max = -INFINITY;
        for (vox[y] = 0; vox[y] < ydim; ++vox[y]) {
          for (vox[x] = 0; vox[x] < xdim; ++vox[x]) {
            float val = vox.value();
            data[vox[x]+vox[y]*xdim] = val;
            if (finite (val)) {
              if (val < value_min) value_min = val;
              if (val > value_max) value_max = val;
            }
          }
        }

        if (isnan (display_midpoint) || isnan (display_range))
          reset_windowing();
      }

      glTexImage2D (GL_TEXTURE_2D, 0, GL_LUMINANCE32F_ARB, xdim, ydim, 0, GL_LUMINANCE, GL_FLOAT, data);
    }





    inline void Image::update_texture3D ()
    {
      if (!texture3D) { // allocate:
        glGenTextures (1, &texture3D);
        assert (texture3D);
        glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
        glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
        glBindTexture (GL_TEXTURE_3D, texture3D);
        glTexImage3D (GL_TEXTURE_3D, 0, GL_LUMINANCE32F_ARB, 
            vox.dim(0), vox.dim(1), vox.dim(3), 
            0, GL_LUMINANCE, GL_FLOAT, NULL);
      }
      else if (volume_unchanged()) {
        glBindTexture (GL_TEXTURE_3D, texture3D);
        return;
      }

      glBindTexture (GL_TEXTURE_3D, texture3D);

      value_min = INFINITY;
      value_max = -INFINITY;
      for (ssize_t k = 0; k < vox.dim (2); ++k) {
        float data [vox.dim(0)*vox.dim(1)];
        float* p = data;
        for (ssize_t j = 0; j < vox.dim (1); ++j) {
          for (ssize_t i = 0; i < vox.dim (0); ++i) {
            float val = *p++ = vox.value();
            if (finite (val)) {
              if (val < value_min) value_min = val;
              if (val > value_max) value_max = val;
            }
          }
        }
        glTexSubImage3D (GL_TEXTURE_3D, 0, 
            0, 0, k, 
            vox.dim(0), vox.dim(1), 1,
            GL_LUMINANCE, GL_FLOAT, data);
      }

      if (isnan (display_midpoint) || isnan (display_range))
        reset_windowing();

      }





    inline bool Image::volume_unchanged ()
    {
      for (size_t i = 3; i < vox.ndim(); ++i)
        if (vox[i] != position[i]) 
          return false;
      return true;
    }



  }
}



