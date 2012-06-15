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

#include "progressbar.h"
#include "image/stride.h"
#include "gui/mrview/image.h"
#include "gui/mrview/window.h"
#include "gui/mrview/mode/base.h"



namespace MR
{
  namespace GUI
  {
    namespace MRView
    {

      Image::Image (Window& parent, const MR::Image::Header& image_header) :
        QAction (shorten (image_header.name(), 20, 0).c_str(), &parent),
        buffer (image_header),
        interp (buffer),
        window (parent),
        texture3D (0),
        interpolation (GL_LINEAR),
        value_min (NAN),
        value_max (NAN),
        display_midpoint (NAN),
        display_range (NAN),
        position (header().ndim()),
        texture_mode_2D_unchanged (false),
        texture_mode_3D_unchanged (false)
      {
        setCheckable (true);
        setToolTip (header().name().c_str());
        setStatusTip (header().name().c_str());
        window.image_group->addAction (this);
        window.image_menu->addAction (this);
        texture2D[0] = texture2D[1] = texture2D[2] = 0;
        position[0] = position[1] = position[2] = std::numeric_limits<ssize_t>::min();
        set_colourmap (header().datatype().is_complex() ? ColourMap::Complex : ColourMap::Gray);
      }

      Image::~Image ()
      {
        glDeleteTextures (3, texture2D);
        glDeleteTextures (1, &texture3D);
      }



      void Image::render2D (Shader& custom_shader, int projection, int slice)
      {
        update_texture2D (custom_shader, projection, slice);

        glTexParameteri (GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, interpolation);
        glTexParameteri (GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, interpolation);

        int x, y;
        get_axes (projection, x, y);
        float xdim = header().dim (x)-0.5, ydim = header().dim (y)-0.5;

        Point<> p, q;
        p[projection] = slice;

        set_color (custom_shader);

        custom_shader.start (display_midpoint, display_range, 
            lessthan, greaterthan, 
            transparent_intensity, opaque_intensity, alpha);

        glBegin (GL_QUADS);
        glTexCoord3f (0.0, 0.0, 0.0);
        p[x] = -0.5;
        p[y] = -0.5;
        q = interp.voxel2scanner (p);
        glVertex3fv (q);
        glTexCoord3f (0.0, 1.0, 0.0);
        p[x] = -0.5;
        p[y] = ydim;
        q = interp.voxel2scanner (p);
        glVertex3fv (q);
        glTexCoord3f (1.0, 1.0, 0.0);
        p[x] = xdim;
        p[y] = ydim;
        q = interp.voxel2scanner (p);
        glVertex3fv (q);
        glTexCoord3f (1.0, 0.0, 0.0);
        p[x] = xdim;
        p[y] = -0.5;
        q = interp.voxel2scanner (p);
        glVertex3fv (q);
        glEnd();
        custom_shader.stop();
      }





      void Image::render3D_pre (Shader& custom_shader, const Mode::Base& mode)
      {
        update_texture3D (custom_shader);

        glTexParameteri (GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, interpolation);
        glTexParameteri (GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, interpolation);

        custom_shader.start (display_midpoint/windowing_scale_3D, display_range/windowing_scale_3D, 
            lessthan/windowing_scale_3D, greaterthan/windowing_scale_3D,
            transparent_intensity/windowing_scale_3D, opaque_intensity/windowing_scale_3D, alpha);

        if (custom_shader.use_lighting())
          glEnable (GL_LIGHTING);

        pos[0] = mode.screen_to_model (QPoint (0, mode.height()));
        pos[1] = mode.screen_to_model (QPoint (0, 0));
        pos[2] = mode.screen_to_model (QPoint (mode.width(), 0));
        pos[3] = mode.screen_to_model (QPoint (mode.width(), mode.height()));

        set_color (custom_shader);
      }




      void Image::render3D_slice (const Mode::Base& mode, float offset)
      {
        Point<> spos[4];

        spos[0] = pos[0];
        spos[1] = pos[1];
        spos[2] = pos[2];
        spos[3] = pos[3];

        if (offset != 0.0) {
          Point<> z = mode.screen_to_model_direction (Point<> (0.0, 0.0, 1.0));
          z.normalise();
          z *= offset;
          spos[0] += z;
          spos[1] += z;
          spos[2] += z;
          spos[3] += z;
        }

        Point<> tex[4];
        tex[0] = interp.scanner2voxel (spos[0]) + Point<> (0.5, 0.5, 0.5);
        tex[1] = interp.scanner2voxel (spos[1]) + Point<> (0.5, 0.5, 0.5);
        tex[2] = interp.scanner2voxel (spos[2]) + Point<> (0.5, 0.5, 0.5);
        tex[3] = interp.scanner2voxel (spos[3]) + Point<> (0.5, 0.5, 0.5);

        for (size_t i = 0; i < 4; ++i)
          for (size_t j = 0; j < 3; ++j)
            tex[i][j] /= header().dim (j);

        glBegin (GL_QUADS);
        glTexCoord3fv (tex[0]);
        glVertex3fv (spos[0]);
        glTexCoord3fv (tex[1]);
        glVertex3fv (spos[1]);
        glTexCoord3fv (tex[2]);
        glVertex3fv (spos[2]);
        glTexCoord3fv (tex[3]);
        glVertex3fv (spos[3]);
        glEnd();
      }




      inline void Image::update_texture2D (const Shader& custom_shader, int projection, int slice)
      {
        if (!texture2D[projection]) { // allocate:
          glGenTextures (1, &texture2D[projection]);
          assert (texture2D[projection]);
          glTexParameteri (GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP);
          glTexParameteri (GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP);
          glTexParameteri (GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP);
        }
        glBindTexture (GL_TEXTURE_3D, texture2D[projection]);

        if (position[projection] == slice && volume_unchanged() && texture_mode_2D_unchanged)
          return;

        position[projection] = slice;
        texture_mode_2D_unchanged = true;

        int x, y;
        get_axes (projection, x, y);
        ssize_t xdim = header().dim (x), ydim = header().dim (y);

        type = GL_FLOAT;
        Ptr<float,true> data;

        uint32_t cmap = custom_shader.colourmap() & ColourMap::Mask;
        if (cmap < ColourMap::Special) {

          data = new float [xdim*ydim];
          format = GL_ALPHA;
          internal_format = GL_ALPHA32F_ARB;

          if (position[projection] < 0 || position[projection] >= header().dim (projection)) {
            memset (data, 0, xdim*ydim*sizeof (float));
          }
          else {
            // copy data:
            VoxelType& vox (voxel());
            vox[projection] = slice;
            value_min = std::numeric_limits<float>::infinity();
            value_max = -std::numeric_limits<float>::infinity();
            for (vox[y] = 0; vox[y] < ydim; ++vox[y]) {
              for (vox[x] = 0; vox[x] < xdim; ++vox[x]) {
                cfloat val = vox.value();
                data[vox[x]+vox[y]*xdim] = val.real();
                if (finite (val.real())) {
                  if (val.real() < value_min) value_min = val.real();
                  if (val.real() > value_max) value_max = val.real();
                }
              }
            }
          }

        }
        else if (cmap == ColourMap::RGB) {

          data = new float [3*xdim*ydim];
          format = GL_RGB;
          internal_format = GL_RGB32F;

          memset (data, 0, 3*xdim*ydim*sizeof (float));
          if (position[projection] >= 0 && position[projection] < header().dim (projection)) {
            // copy data:
            VoxelType& vox (voxel());
            vox[projection] = slice;
            value_min = std::numeric_limits<float>::infinity();
            value_max = -std::numeric_limits<float>::infinity();

            for (size_t n = 0; n < 3; ++n) {
              if (vox.ndim() > 3) {
                if (vox.dim(3) > int(position[3] + n))
                  vox[3] = position[3] + n;
                else break;
              }
              for (vox[y] = 0; vox[y] < ydim; ++vox[y]) {
                for (vox[x] = 0; vox[x] < xdim; ++vox[x]) {
                  cfloat val = vox.value();
                  float mag = Math::abs (val.real());
                  data[3*(vox[x]+vox[y]*xdim) + n] = mag;
                  if (finite (mag)) {
                    if (mag < value_min) value_min = mag;
                    if (mag > value_max) value_max = mag;
                  }
                }
              }

              if (vox.ndim() <= 3) 
                break;
            }
            if (vox.ndim() > 3) 
              vox[3] = position[3];
          }

        }
        else if (cmap == ColourMap::Complex) {

          data = new float [2*xdim*ydim];
          format = GL_LUMINANCE_ALPHA;
          internal_format = GL_LUMINANCE_ALPHA32F_ARB;

          if (position[projection] < 0 || position[projection] >= header().dim (projection)) {
            memset (data, 0, 2*xdim*ydim*sizeof (float));
          }
          else {
            // copy data:
            VoxelType& vox (voxel());
            vox[projection] = slice;
            value_min = std::numeric_limits<float>::infinity();
            value_max = -std::numeric_limits<float>::infinity();
            for (vox[y] = 0; vox[y] < ydim; ++vox[y]) {
              for (vox[x] = 0; vox[x] < xdim; ++vox[x]) {
                cfloat val = vox.value();
                size_t idx = 2*(vox[x]+vox[y]*xdim);
                data[idx] = val.real();
                data[idx+1] = val.imag();
                float mag = std::abs (val);
                if (finite (mag)) {
                  if (mag < value_min) value_min = mag;
                  if (mag > value_max) value_max = mag;
                }
              }
            }
          }

        }
        else {
          error ("attempt to use unsupported colourmap");
          return;
        }
        if ((value_max - value_min) < 2.0*std::numeric_limits<float>::epsilon()) 
          value_min = value_max - 1.0;

        if (isnan (display_midpoint) || isnan (display_range))
          reset_windowing();

        glTexImage3D (GL_TEXTURE_3D, 0, internal_format, xdim, ydim, 1, 0, format, type, data);
      }







      inline void Image::update_texture3D (const Shader& custom_shader)
      {
        uint32_t cmap = custom_shader.colourmap() & ColourMap::Mask;

        if (cmap < ColourMap::Special) format = GL_ALPHA;
        else if (cmap == ColourMap::RGB) format = GL_RGB;
        else if (cmap == ColourMap::Complex) format = GL_LUMINANCE_ALPHA;
        else error ("attempt to use unsupported colourmap");

        if (cmap == ColourMap::Complex) 
          internal_format = GL_LUMINANCE_ALPHA32F_ARB;
        else {

          switch (header().datatype() ()) {
            case DataType::Bit:
            case DataType::UInt8:
            case DataType::Int8:
              if (cmap < ColourMap::Special) internal_format = GL_ALPHA8;
              else if (cmap == ColourMap::Complex) internal_format = GL_LUMINANCE8_ALPHA8;
              else if (cmap == ColourMap::RGB) internal_format = GL_RGB8;
              else error ("attempt to use unsupported colourmap");
              break;
            case DataType::UInt16LE:
            case DataType::UInt16BE:
            case DataType::Int16LE:
            case DataType::Int16BE:
              if (cmap < ColourMap::Special) internal_format = GL_ALPHA16;
              else if (cmap == ColourMap::Complex) internal_format = GL_LUMINANCE16_ALPHA16;
              else if (cmap == ColourMap::RGB) internal_format = GL_RGB16;
              else error ("attempt to use unsupported colourmap");
              break;
            default:
              if (cmap < ColourMap::Special) internal_format = GL_ALPHA32F_ARB;
              else if (cmap == ColourMap::RGB) internal_format = GL_RGB32F;
              else if (cmap == ColourMap::Complex) internal_format = GL_LUMINANCE_ALPHA32F_ARB;
              else error ("attempt to use unsupported colourmap");
              break;
          }
        }

        if (volume_unchanged() && texture_mode_3D_unchanged) {
          glBindTexture (GL_TEXTURE_3D, texture3D);
          return;
        }

        if (!texture3D) { // allocate:
          glGenTextures (1, &texture3D);
          assert (texture3D);
          glBindTexture (GL_TEXTURE_3D, texture3D);
          glTexParameteri (GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP);
          glTexParameteri (GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP);
          glTexParameteri (GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP);
          DEBUG_OPENGL;
        }

        texture_mode_3D_unchanged = true;

        glBindTexture (GL_TEXTURE_3D, texture3D);

        glTexImage3D (GL_TEXTURE_3D, 0, internal_format,
            header().dim (0), header().dim (1), header().dim (2),
            0, format, GL_FLOAT, NULL);

        value_min = std::numeric_limits<float>::infinity();
        value_max = -std::numeric_limits<float>::infinity();

        if (cmap != ColourMap::Complex) {
          switch (header().datatype() ()) {
            case DataType::Bit:
            case DataType::UInt8:
              copy_texture_3D<uint8_t> (format);
              break;
            case DataType::Int8:
              copy_texture_3D<int8_t> (format);
              break;
            case DataType::UInt16LE:
            case DataType::UInt16BE:
              copy_texture_3D<uint16_t> (format);
              break;
            case DataType::Int16LE:
            case DataType::Int16BE:
              copy_texture_3D<int16_t> (format);
              break;
            case DataType::UInt32LE:
            case DataType::UInt32BE:
              copy_texture_3D<uint32_t> (format);
              break;
            case DataType::Int32LE:
            case DataType::Int32BE:
              copy_texture_3D<int32_t> (format);
              break;
            case DataType::Float32LE:
            case DataType::Float32BE:
            case DataType::Float64LE:
            case DataType::Float64BE:
              copy_texture_3D<float> (format);
              break;
            default:
              assert (0);
          }
        }
        else 
          copy_texture_3D_complex();

        if (isnan (display_midpoint) || isnan (display_range))
          reset_windowing();
      }

      template <> inline GLenum Image::GLtype<int8_t> () const
      {
        return GL_BYTE;
      }
      template <> inline GLenum Image::GLtype<uint8_t> () const
      {
        return GL_UNSIGNED_BYTE;
      }
      template <> inline GLenum Image::GLtype<int16_t> () const
      {
        return GL_SHORT;
      }
      template <> inline GLenum Image::GLtype<uint16_t> () const
      {
        return GL_UNSIGNED_SHORT;
      }
      template <> inline GLenum Image::GLtype<int32_t> () const
      {
        return GL_INT;
      }
      template <> inline GLenum Image::GLtype<uint32_t> () const
      {
        return GL_UNSIGNED_INT;
      }
      template <> inline GLenum Image::GLtype<float> () const
      {
        return GL_FLOAT;
      }

      template <typename ValueType> inline float Image::scale_factor_3D () const
      {
        return std::numeric_limits<ValueType>::max();
      }
      template <> inline float Image::scale_factor_3D<float> () const
      {
        return 1.0;
      }


      template <typename ValueType>
        inline void Image::copy_texture_3D (GLenum format)
      {
        MR::Image::Buffer<ValueType> buffer_tmp (buffer);
        typename MR::Image::Buffer<ValueType>::voxel_type V (buffer_tmp);
        GLenum type = GLtype<ValueType>();
        int N = ( format == GL_ALPHA ? 1 : 3 );
        Ptr<ValueType,true> data (new ValueType [N * V.dim (0) * V.dim (1)]);

        ProgressBar progress ("loading image data...", V.dim (2));

        for (size_t n = 3; n < V.ndim(); ++n) 
          V[n] = interp[n];

        for (V[2] = 0; V[2] < V.dim (2); ++V[2]) {

          if (format == GL_ALPHA) {
            ValueType* p = data;

            for (V[1] = 0; V[1] < V.dim (1); ++V[1]) {
              for (V[0] = 0; V[0] < V.dim (0); ++V[0]) {
                ValueType val = *p = V.value();
                if (finite (val)) {
                  if (val < value_min) value_min = val;
                  if (val > value_max) value_max = val;
                }
                ++p;
              }
            }

          }
          else {

            memset (data, 0, 3*V.dim(0)*V.dim(1)*sizeof (ValueType));
            for (size_t n = 0; n < 3; ++n) {
              if (V.ndim() > 3) {
                if (V.dim(3) > int(position[3] + n))
                  V[3] = position[3] + n;
                else break;
              }

              ValueType* p = data + n;
              for (V[1] = 0; V[1] < V.dim (1); ++V[1]) {
                for (V[0] = 0; V[0] < V.dim (0); ++V[0]) {
                  ValueType val = *p = Math::abs (ValueType (V.value()));
                  if (finite (val)) {
                    if (val < value_min) value_min = val;
                    if (val > value_max) value_max = val;
                  }
                  p += 3;
                }
              }

              if (V.ndim() <= 3) 
                break;
            }
            if (V.ndim() > 3) 
              V[3] = position[3];

          }

          glTexSubImage3D (GL_TEXTURE_3D, 0,
              0, 0, V[2],
              V.dim (0), V.dim (1), 1,
              format, type, data);
          DEBUG_OPENGL;
          ++progress;
        }

        windowing_scale_3D = scale_factor_3D<ValueType>();
      }



      inline void Image::copy_texture_3D_complex ()
      {
        MR::Image::Buffer<cfloat>::voxel_type V (buffer);
        Ptr<float,true> data (new float [2 * V.dim (0) * V.dim (1)]);

        ProgressBar progress ("loading image data...", V.dim (2));

        for (size_t n = 3; n < V.ndim(); ++n) 
          V[n] = interp[n];

        for (V[2] = 0; V[2] < V.dim (2); ++V[2]) {
          float* p = data;

          for (V[1] = 0; V[1] < V.dim (1); ++V[1]) {
            for (V[0] = 0; V[0] < V.dim (0); ++V[0]) {
              cfloat val = V.value();
              *(p++) = val.real();
              *(p++) = val.imag();
              float mag = std::abs (val);
              if (finite (mag)) {
                if (mag < value_min) value_min = mag;
                if (mag > value_max) value_max = mag;
              }
            }
          }

          glTexSubImage3D (GL_TEXTURE_3D, 0,
              0, 0, V[2],
              V.dim (0), V.dim (1), 1,
              GL_LUMINANCE_ALPHA, GL_FLOAT, data);
          DEBUG_OPENGL;
          ++progress;
        }

        windowing_scale_3D = scale_factor_3D<float>();
      }



      inline bool Image::volume_unchanged ()
      {
        bool is_unchanged = true;
        for (size_t i = 3; i < interp.ndim(); ++i) {
          if (interp[i] != position[i]) {
            is_unchanged = false;
            position[i] = interp[i];
          }
        }

        if (!is_unchanged) 
          position[0] = position[1] = position[2] = -1;

        return is_unchanged;
      }

      void Image::adjust_windowing (float brightness, float contrast) {
        display_midpoint -= 0.0005f * display_range * brightness;
        display_range *= Math::exp (0.002f * contrast);
        window.scaling_updated();
      }

      void Image::set_windowing (float min, float max) {
        display_range = max - min;
        display_midpoint = 0.5 * (min + max);
        window.scaling_updated();
      }

    }
  }
}


