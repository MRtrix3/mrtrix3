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
#include "gui/projection.h"



namespace MR
{
  namespace GUI
  {
    namespace MRView
    {

      Image::Image (const MR::Image::Header& image_header) :
        Displayable (image_header.name()),
        buffer (image_header),
        interp (buffer),
        texture3D_ID (0),
        position (header().ndim()),
        vertex_buffer_ID (0),
        vertex_array_object_ID (0)
      {
        texture2D_ID[0] = texture2D_ID[1] = texture2D_ID[2] = 0;
        position[0] = position[1] = position[2] = std::numeric_limits<ssize_t>::min();
        set_colourmap (header().datatype().is_complex() ? ColourMap::Complex : ColourMap::Gray);
      }



      Image::Image (Window& window, const MR::Image::Header& image_header) :
        Displayable (window, image_header.name()),
        buffer (image_header),
        interp (buffer),
        texture3D_ID (0),
        position (image_header.ndim()),
        vertex_buffer_ID (0),
        vertex_array_object_ID (0)
      {
        texture2D_ID[0] = texture2D_ID[1] = texture2D_ID[2] = 0;
        position[0] = position[1] = position[2] = std::numeric_limits<ssize_t>::min();
        set_colourmap (header().datatype().is_complex() ? ColourMap::Complex : ColourMap::Gray);
        setCheckable (true);
        setToolTip (header().name().c_str());
        setStatusTip (header().name().c_str());
        window.image_group->addAction (this);
        window.image_menu->addAction (this);
        connect (this, SIGNAL(scalingChanged()), &window, SLOT(on_scaling_changed()));
      }


      Image::~Image ()
      {
        if (texture2D_ID[0]) 
          glDeleteTextures (3, texture2D_ID);
        if (texture3D_ID)
          glDeleteTextures (1, &texture3D_ID);
        if (vertex_buffer_ID)
          glDeleteBuffers (1, &vertex_buffer_ID);
        if (vertex_array_object_ID)
          glDeleteVertexArrays (1, &vertex_array_object_ID);
      }




      inline void Image::draw_vertices (const Point<float>* vertices)
      {
        if (!vertex_buffer_ID || !vertex_array_object_ID) {
          assert (vertex_buffer_ID == 0);
          assert (vertex_array_object_ID == 0);

          glGenBuffers (1, &vertex_buffer_ID);
          glGenVertexArrays (1, &vertex_array_object_ID);

          glBindBuffer (GL_ARRAY_BUFFER, vertex_buffer_ID);
          glBindVertexArray (vertex_array_object_ID);

          glEnableVertexAttribArray (0);
          glVertexAttribPointer (0, 3, GL_FLOAT, GL_FALSE, 2*sizeof(Point<float>), (void*)0);

          glEnableVertexAttribArray (1);
          glVertexAttribPointer (1, 3, GL_FLOAT, GL_FALSE, 2*sizeof(Point<float>), (void*)(sizeof(Point<float>)));
        }
        else {
          glBindBuffer (GL_ARRAY_BUFFER, vertex_buffer_ID);
          glBindVertexArray (vertex_array_object_ID);
        }

        glBufferData (GL_ARRAY_BUFFER, 8*sizeof(Point<float>), &vertices[0][0], GL_STREAM_DRAW);
        glDrawArrays (GL_QUADS, 0, 4);
      }





      void Image::render2D (const Projection& projection, int plane, int slice)
      {
        update_texture2D (plane, slice);

        glTexParameteri (GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, interpolation);
        glTexParameteri (GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, interpolation);

        int x, y;
        get_axes (plane, x, y);
        float xdim = header().dim (x)-0.5, ydim = header().dim (y)-0.5;

        Point<> p, q;
        p[plane] = slice;

        Point<float> vertices[8];
        p[x] = -0.5;
        p[y] = -0.5;
        vertices[0] = interp.voxel2scanner (p);
        vertices[1].set (0.0, 0.0, 0.0);

        p[x] = -0.5;
        p[y] = ydim;
        vertices[2] = interp.voxel2scanner (p);
        vertices[3].set (0.0, 1.0, 0.0);

        p[x] = xdim;
        p[y] = ydim;
        vertices[4] = interp.voxel2scanner (p);
        vertices[5].set (1.0, 1.0, 0.0);

        p[x] = xdim;
        p[y] = -0.5;
        vertices[6] = interp.voxel2scanner (p);
        vertices[7].set (1.0, 0.0, 0.0);


        shader.start (projection);
        draw_vertices (vertices);
        shader.stop();
      }





      void Image::render3D_pre (const Projection& projection, float depth)
      {
        update_texture3D();

        glTexParameteri (GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, interpolation);
        glTexParameteri (GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, interpolation);

        shader.start (projection, windowing_scale_3D);

        if (shader.use_lighting())
          glEnable (GL_LIGHTING);

        pos[0] = projection.screen_to_model (0.0, projection.height(), depth);
        pos[1] = projection.screen_to_model (0.0, 0.0, depth);
        pos[2] = projection.screen_to_model (projection.width(), 0.0, depth);
        pos[3] = projection.screen_to_model (projection.width(), projection.height(), depth);

        tex[0] = interp.scanner2voxel (pos[0]) + Point<> (0.5, 0.5, 0.5);
        tex[1] = interp.scanner2voxel (pos[1]) + Point<> (0.5, 0.5, 0.5);
        tex[2] = interp.scanner2voxel (pos[2]) + Point<> (0.5, 0.5, 0.5);
        tex[3] = interp.scanner2voxel (pos[3]) + Point<> (0.5, 0.5, 0.5);

        z = projection.screen_normal();
        im_z = interp.scanner2voxel_dir (z);
      }




      void Image::render3D_slice (float offset)
      {
        Point<> vertices[8];

        if (offset == 0.0) {
          vertices[0] = pos[0];
          vertices[2] = pos[1];
          vertices[4] = pos[2];
          vertices[6] = pos[3];
          vertices[1] = tex[0];
          vertices[3] = tex[1];
          vertices[5] = tex[2];
          vertices[7] = tex[3];
        }
        else {
          Point<> d = z * offset;
          vertices[0] = pos[0] + d;
          vertices[2] = pos[1] + d;
          vertices[4] = pos[2] + d;
          vertices[6] = pos[3] + d;
          d = im_z * offset;
          vertices[1] = tex[0] + d;
          vertices[3] = tex[1] + d;
          vertices[5] = tex[2] + d;
          vertices[7] = tex[3] + d;
        }

        for (size_t i = 0; i < 4; ++i)
          for (size_t j = 0; j < 3; ++j)
            vertices[2*i+1][j] /= header().dim (j);

        draw_vertices (vertices);
      }




      inline void Image::update_texture2D (int plane, int slice)
      {
        if (!texture2D_ID[plane]) { // allocate:
          glGenTextures (1, &texture2D_ID[plane]);
          assert (texture2D_ID[plane]);
          glBindTexture (GL_TEXTURE_3D, texture2D_ID[plane]);
          glTexParameteri (GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
          glTexParameteri (GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
          glTexParameteri (GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
        }
        glBindTexture (GL_TEXTURE_3D, texture2D_ID[plane]);

        if (position[plane] == slice && volume_unchanged())
          return;

        position[plane] = slice;

        int x, y;
        get_axes (plane, x, y);
        ssize_t xdim = header().dim (x), ydim = header().dim (y);

        type = GL_FLOAT;
        Ptr<float,true> data;

        uint32_t cmap = shader.colourmap() & ColourMap::Mask;
        if (cmap < ColourMap::Special) {

          data = new float [xdim*ydim];
          format = GL_ALPHA;
          internal_format = GL_ALPHA32F_ARB;

          if (position[plane] < 0 || position[plane] >= header().dim (plane)) {
            memset (data, 0, xdim*ydim*sizeof (float));
          }
          else {
            // copy data:
            VoxelType& vox (voxel());
            vox[plane] = slice;
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
          if (position[plane] >= 0 && position[plane] < header().dim (plane)) {
            // copy data:
            VoxelType& vox (voxel());
            vox[plane] = slice;
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

          if (position[plane] < 0 || position[plane] >= header().dim (plane)) {
            memset (data, 0, 2*xdim*ydim*sizeof (float));
          }
          else {
            // copy data:
            VoxelType& vox (voxel());
            vox[plane] = slice;
            value_min = std::numeric_limits<float>::infinity();
            value_max = -std::numeric_limits<float>::infinity();
            for (vox[y] = 0; vox[y] < ydim; ++vox[y]) {
              for (vox[x] = 0; vox[x] < xdim; ++vox[x]) {
                cfloat val = vox.value();
                size_t idx = 2*(vox[x]+vox[y]*xdim);
                data[idx] = val.real();
                data[idx+1] = val.imag();
                float mag = std::abs (val);
                if (finite (mag)) 
                  if (mag > value_max) 
                    value_max = mag;
              }
            }
          }
          value_min = 0.0;

        }
        else {
          ERROR ("attempt to use unsupported colourmap");
          return;
        }
        if ((value_max - value_min) < 2.0*std::numeric_limits<float>::epsilon()) 
          value_min = value_max - 1.0;

        if (isnan (shader.display_midpoint) || isnan (shader.display_range))
          reset_windowing();

        glTexImage3D (GL_TEXTURE_3D, 0, internal_format, xdim, ydim, 1, 0, format, type, data);
      }







      inline void Image::update_texture3D ()
      {
        uint32_t cmap = shader.colourmap() & ColourMap::Mask;

        if (cmap < ColourMap::Special) format = GL_ALPHA;
        else if (cmap == ColourMap::RGB) format = GL_RGB;
        else if (cmap == ColourMap::Complex) format = GL_LUMINANCE_ALPHA;
        else ERROR ("attempt to use unsupported colourmap");

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
              else ERROR ("attempt to use unsupported colourmap");
              break;
            case DataType::UInt16LE:
            case DataType::UInt16BE:
            case DataType::Int16LE:
            case DataType::Int16BE:
              if (cmap < ColourMap::Special) internal_format = GL_ALPHA16;
              else if (cmap == ColourMap::Complex) internal_format = GL_LUMINANCE16_ALPHA16;
              else if (cmap == ColourMap::RGB) internal_format = GL_RGB16;
              else ERROR ("attempt to use unsupported colourmap");
              break;
            default:
              if (cmap < ColourMap::Special) internal_format = GL_ALPHA32F_ARB;
              else if (cmap == ColourMap::RGB) internal_format = GL_RGB32F;
              else if (cmap == ColourMap::Complex) internal_format = GL_LUMINANCE_ALPHA32F_ARB;
              else ERROR ("attempt to use unsupported colourmap");
              break;
          }
        }

        if (volume_unchanged() && texture_mode_3D_unchanged) {
          glBindTexture (GL_TEXTURE_3D, texture3D_ID);
          return;
        }

        if (!texture3D_ID) { // allocate:
          glGenTextures (1, &texture3D_ID);
          assert (texture3D_ID);
          glBindTexture (GL_TEXTURE_3D, texture3D_ID);
          glTexParameteri (GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
          glTexParameteri (GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
          glTexParameteri (GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
          DEBUG_OPENGL;
        }

        texture_mode_3D_unchanged = true;

        glBindTexture (GL_TEXTURE_3D, texture3D_ID);

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

        if (isnan (shader.display_midpoint) || isnan (shader.display_range))
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


    }
  }
}


