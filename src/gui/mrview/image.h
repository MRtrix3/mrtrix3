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

#include "image/buffer.h"
#include "image/voxel.h"
#include "math/quaternion.h"
#include "image/interp/linear.h"
#include "gui/mrview/shader.h"

class QAction;

namespace MR
{
  class ProgressBar;

  namespace GUI
  {
    namespace MRView
    {

      class Window;
      class Transform;

      class Image : public QAction
      {
          Q_OBJECT

        public:
          Image (Window& parent, const MR::Image::Header& image_header);
          ~Image ();

          MR::Image::Header& header () {
            return buffer;
          }
          const MR::Image::Header& header () const {
            return buffer;
          }

          float scaling_min () const {
            return display_midpoint - 0.5f * display_range;
          }
          float scaling_max () const {
            return display_midpoint + 0.5f * display_range;
          }

          float intensity_min () const {
            return value_min;
          }
          float intensity_max () const {
            return value_max;
          }

          void set_windowing (float min, float max);
          void reset_windowing () {
            set_windowing (value_min, value_max);
          }

          void adjust_windowing (float brightness, float contrast);
          void adjust_windowing (const QPoint& p) {
            adjust_windowing (p.x(), p.y());
          }
          void set_interpolate (bool linear) {
            interpolation = linear ? GL_LINEAR : GL_NEAREST;
          }
          bool interpolate () const {
            return interpolation == GL_LINEAR;
          }

          void render2D (Shader& custom_shader, int projection, int slice);
          void render2D (int projection, int slice) {
            render2D (shader, projection, slice);
          }

          void render3D_pre (Shader& custom_shader, const Transform& transform, float depth);
          void render3D_slice (float offset);
          void render3D_post (Shader& custom_shader) {
            custom_shader.stop(); 
            if (custom_shader.use_lighting())
              glDisable (GL_LIGHTING);
          }
          void render3D_post () {
            render3D_post (shader);
          }

          void render3D_pre (const Transform& transform, float depth) {
            render3D_pre (shader, transform, depth);
          }
          void render3D (const Transform& transform, float depth) {
            render3D (shader, transform, depth);
          }

          void render3D (Shader& custom_shader, const Transform& transform, float depth) {
            render3D_pre (custom_shader, transform, depth);
            render3D_slice (0.0);
            render3D_post (custom_shader);
          }


          void get_axes (int projection, int& x, int& y) {
            if (projection) {
              if (projection == 1) {
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
              if (index != shader.colourmap())
                texture_mode_2D_unchanged = texture_mode_3D_unchanged = false;
            } 
            shader.set_colourmap (index);
          }
          void set_invert_map (bool value) { 
            shader.set_invert_map (value); 
          }
          void set_invert_scale (bool value) { 
            shader.set_invert_scale (value); 
          }
          void set_use_lighting (bool value) { 
            shader.set_use_lighting (value); 
          }

          void set_thresholds (float less_than_value = NAN, float greater_than_value = NAN) {
            lessthan = less_than_value;
            greaterthan = greater_than_value;
            shader.set_use_thresholds (finite (lessthan), finite (greaterthan));
          }

          void set_transparency (float transparent = NAN, float opaque = NAN, float alpha_value = 1.0) {
            transparent_intensity = transparent;
            opaque_intensity = opaque;
            alpha = alpha_value;
            shader.set_use_transparency (finite (transparent_intensity) && finite (opaque_intensity) && finite (alpha));
          }

          bool scale_inverted () const { 
            return shader.scale_inverted();
          }

          bool colourmap_inverted () const {
            return shader.colourmap_inverted();
          }

          uint32_t colourmap_index () const {
            uint32_t cret = shader.colourmap();
            if (cret >= ColourMap::Special)
              cret -= ColourMap::Special - ColourMap::NumScalar;
            return cret;
          }

          typedef MR::Image::Buffer<cfloat> BufferType;
          typedef BufferType::voxel_type VoxelType;
          typedef MR::Image::Interp::Linear<VoxelType> InterpVoxelType;

        private:
          BufferType buffer;
        public:
          InterpVoxelType interp;
          VoxelType& voxel () { 
            return interp;
          }
        private:
          Window& window;
          GLuint texture2D[3], texture3D;
          int interpolation;
          float value_min, value_max, lessthan, greaterthan;
          float display_midpoint, display_range;
          float transparent_intensity, opaque_intensity, alpha;
          float windowing_scale_3D;
          GLenum type, format, internal_format;
          std::vector<ssize_t> position;
          bool texture_mode_2D_unchanged, texture_mode_3D_unchanged;
          Point<> pos[4], tex[4], z, im_z;

          Shader shader;

          void update_texture2D (const Shader& custom_shader, int projection, int slice);
          void update_texture3D (const Shader& custom_shader);

          bool volume_unchanged ();

          void set_color (const Shader& custom_shader) {
            if (custom_shader.colourmap() == ColourMap::DWI) {
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

          friend class Window;
      };

    }
  }
}

#endif

