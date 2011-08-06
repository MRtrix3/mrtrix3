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

#ifndef __viewer_image_h__
#define __viewer_image_h__

#include <QAction>

#include "mrview/shader.h"
#include "image/voxel.h"
#include "math/quaternion.h"
#include "dataset/interp/linear.h"

class QAction;

namespace MR {
  class ProgressBar;

  namespace Viewer {

    namespace Mode {
      class Base;
    }

    class Window;

    class Image : public QAction
    {
      Q_OBJECT

      private:
        Ptr<MR::Image::Header> H;

      public:
        Image (Window& parent, MR::Image::Header* image_header);
        ~Image ();

        MR::Image::Header& header () { assert (H); return (*H); }
        const MR::Image::Header& header () const { assert (H); return (*H); }

        void reset_windowing ()
        { 
          display_range = value_max - value_min;
          display_midpoint = 0.5 * (value_min + value_max);
        }

        void adjust_windowing (float brightness, float contrast) 
        {
          display_midpoint -= 0.0005f * display_range * brightness;
          display_range *= Math::exp (0.002f * contrast);
        }
        void adjust_windowing (const QPoint& p) { adjust_windowing (p.x(), p.y()); }
        void set_interpolate (bool linear) { interpolation = linear ? GL_LINEAR : GL_NEAREST; }
        bool interpolate () const { return interpolation == GL_LINEAR; }

        void render2D (int projection, int slice) { render2D (shader2D, projection, slice); }
        void render3D (const Mode::Base& mode) { render3D (shader3D, mode); }

        void render2D (Shader& shader, int projection, int slice);
        void render3D (Shader& shader, const Mode::Base& mode);

        void get_axes (int projection, int& x, int& y)
        { 
          if (projection) {
            if (projection == 1) { x = 0; y = 2; }
            else { x = 0; y = 1; }
          }
          else { x = 1; y = 2; }
        }

        void set_colourmap (uint32_t index, bool invert_scale, bool invert_map)
        { 
          colourmap = index; 
          if (invert_scale) colourmap |= InvertScale;
          if (invert_map) colourmap |= InvertMap;
          shader2D.set (Texture2D | colourmap);
          shader3D.set (Texture3D | colourmap);
        }


        MR::Image::Voxel<float> vox;
        MR::DataSet::Interp::Linear<MR::Image::Voxel<float> > interp;

      private:
        Window& window;
        GLuint texture2D[3], texture3D;
        int interpolation;
        float value_min, value_max;
        float display_midpoint, display_range;
        float windowing_scale_3D;
        uint32_t colourmap;
        GLenum type, format, internal_format;
        std::vector<ssize_t> position;

        Shader shader2D, shader3D;

        void update_texture2D (int projection, int slice);
        void update_texture3D ();

        bool volume_unchanged ();

        template <typename T> void copy_texture_3D (GLenum format);
        template <typename T> GLenum GLtype () const;
        template <typename T> float scale_factor_3D () const;

        friend class Window;
    };

  }
}

#endif

