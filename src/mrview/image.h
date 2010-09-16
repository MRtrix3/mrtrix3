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

#include "opengl/gl.h"
#include "opengl/shader.h"
#include "image/voxel.h"
#include "math/quaternion.h"
#include "dataset/interp/linear.h"

class QAction;

namespace MR {
  namespace Viewer {

    class Window;

    class Image : public QAction
    {
      Q_OBJECT

      public:
        Image (Window& parent, MR::Image::Header* header);
        ~Image ();

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
        bool interpolate () const { return (interpolation == GL_LINEAR); }

        void render2D (int projection, int slice);
        void render3D (const Math::Quaternion& view, const Point<>& focus);

        void get_axes (int projection, int& x, int& y)
        { 
          if (projection) {
            if (projection == 1) { x = 0; y = 2; }
            else { x = 0; y = 1; }
          }
          else { x = 1; y = 2; }
        }

        MR::Image::Header& H;
        MR::Image::Voxel<float> vox;
        MR::DataSet::Interp::Linear<MR::Image::Voxel<float> > interp;

      private:
        Window& window;
        GLuint texture2D[3], texture3D;
        int interpolation;
        float value_min, value_max;
        float display_midpoint, display_range;
        std::vector<ssize_t> position;

        GL::Shader::Vertex vertex_shader;

        GL::Shader::Fragment fragment_shader_2D;
        GL::Shader::Program shader_program_2D;

        GL::Shader::Fragment fragment_shader_3D;
        GL::Shader::Program shader_program_3D;

        const std::string gen_fragment_shader_source_2D () const;
        const std::string gen_fragment_shader_source_3D () const;
        static const char* vertex_shader_source;

        void update_texture2D (int projection, int slice);
        void update_shaders_2D ();

        void update_texture3D ();
        void update_shaders_3D ();

        bool volume_unchanged ();

        friend class Window;
    };

  }
}

#endif

