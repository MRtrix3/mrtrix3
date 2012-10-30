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

#ifndef __gui_mrview_image_h__
#define __gui_mrview_image_h__

#include <QAction>

// necessary to avoid conflict with Qt4's foreach macro:
#ifdef foreach
# undef foreach
#endif

#include "image/buffer.h"
#include "image/voxel.h"
#include "math/versor.h"
#include "image/interp/linear.h"
#include "gui/mrview/shader.h"

class QAction;

namespace MR
{
  class ProgressBar;

  namespace GUI
  {
    class Projection;

    namespace MRView
    {

      class Window;

      class Image : public QAction
      {
        Q_OBJECT

        public:
          Image (const MR::Image::Header& image_header);
          Image (Window& parent, const MR::Image::Header& image_header);

          ~Image ();

          MR::Image::Header& header () {
            return buffer;
          }
          const MR::Image::Header& header () const {
            return buffer;
          }

          float scaling_min () const {
            return shader.display_midpoint - 0.5f * shader.display_range;
          }
          float scaling_max () const {
            return shader.display_midpoint + 0.5f * shader.display_range;
          }

          float intensity_min () const {
            return value_min;
          }
          float intensity_max () const {
            return value_max;
          }

          void set_windowing (float min, float max) {
            shader.display_range = max - min;
            shader.display_midpoint = 0.5 * (min + max);
            emit scalingChanged();
          }
          void adjust_windowing (const QPoint& p) {
            adjust_windowing (p.x(), p.y());
          }
          void set_interpolate (bool linear) {
            interpolation = linear ? GL_LINEAR : GL_NEAREST;
          }
          bool interpolate () const {
            return interpolation == GL_LINEAR;
          }
          void reset_windowing () {
            set_windowing (value_min, value_max);
          }

          void adjust_windowing (float brightness, float contrast) { 
            shader.display_midpoint -= 0.0005f * shader.display_range * brightness;
            shader.display_range *= Math::exp (-0.002f * contrast);
            emit scalingChanged();
          }

          void update_texture2D (int plane, int slice);
          void update_texture3D ();

          void render2D (int plane, int slice);

          void render3D_pre (const Projection& transform, float depth);
          void render3D_slice (float offset);
          void render3D_post () {
            shader.stop(); 
            if (shader.use_lighting())
              glDisable (GL_LIGHTING);
          }

          void render3D (const Projection& transform, float depth) {
            render3D_pre (transform, depth);
            render3D_slice (0.0);
            render3D_post ();
          }


          void get_axes (int plane, int& x, int& y) {
            if (plane) {
              if (plane == 1) {
                x = 0;
                y = 2;
              }
              else {
                x = 0;
                y = 1;
              }
            }
            else {
              x = 1;
              y = 2;
            }
          }

          void set_colourmap (uint32_t index) {
            if (index >= ColourMap::Special || shader.colourmap() >= ColourMap::Special) {
              if (index != shader.colourmap()) {
                position[0] = position[1] = position[2] = std::numeric_limits<ssize_t>::min();
                texture_mode_3D_unchanged = false;
              }
            } 
            shader.set_colourmap (index);
          }


          uint32_t colourmap_index () const {
            uint32_t cret = shader.colourmap();
            if (cret >= ColourMap::Special)
              cret -= ColourMap::Special - ColourMap::NumScalar;
            return cret;
          }

          float scaling_rate () const {
            return 1e-3 * (value_max - value_min);
          }
          
          float focus_rate () const {
            return 1e-3 * (Math::pow ( 
                  interp.dim(0)*interp.vox(0) *
                  interp.dim(1)*interp.vox(1) *
                  interp.dim(2)*interp.vox(2),
                  float (1.0/3.0)));
          }

          typedef MR::Image::Buffer<cfloat> BufferType;
          typedef BufferType::voxel_type VoxelType;
          typedef MR::Image::Interp::Linear<VoxelType> InterpVoxelType;

        signals:
          void scalingChanged ();

        private:
          BufferType buffer;

        public:
          InterpVoxelType interp;
          Shader shader;

          VoxelType& voxel () { 
            return interp;
          }

        private:
          GLuint texture2D[3], texture3D;
          int interpolation;
          float value_min, value_max;
          float windowing_scale_3D;
          GLenum type, format, internal_format;
          std::vector<ssize_t> position;
          bool texture_mode_3D_unchanged;
          Point<> pos[4], tex[4], z, im_z;


          bool volume_unchanged ();

          void set_color () {
            if (shader.colourmap() == ColourMap::DWI) {
              Point<> dir (1.0, 1.0, 1.0);
              if (interp.ndim() > 3) {
                size_t vol = interp[3];
                Math::Matrix<float>& M = header().DW_scheme();
                if (M.rows() > vol) {
                  if (M(vol,3) > 0.0) {
                    dir = Point<> (M(vol,0), M(vol,1), M(vol,2));
                    dir.normalise();
                    dir[0] = Math::abs (dir[0]);
                    dir[1] = Math::abs (dir[1]);
                    dir[2] = Math::abs (dir[2]);
                  }
                }
              }
              glColor3fv (dir);
            }
          }

          template <typename T> void copy_texture_3D (GLenum format);
          void copy_texture_3D_complex ();
          template <typename T> GLenum GLtype () const;
          template <typename T> float scale_factor_3D () const;

      };


    }
  }
}

#endif

