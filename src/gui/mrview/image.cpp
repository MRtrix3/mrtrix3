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
        interpolation (GL_LINEAR),
        texture_mode_3D_unchanged (false),
        position (header().ndim())
      {
        position[0] = position[1] = position[2] = std::numeric_limits<ssize_t>::min();
        set_colourmap (guess_colourmap ());
      }



      Image::Image (Window& window, const MR::Image::Header& image_header) :
        Displayable (window, image_header.name()),
        buffer (image_header),
        interp (buffer),
        interpolation (GL_LINEAR),
        texture_mode_3D_unchanged (false),
        position (image_header.ndim())
      {
        position[0] = position[1] = position[2] = std::numeric_limits<ssize_t>::min();
        set_colourmap (guess_colourmap ());
        setCheckable (true);
        setToolTip (header().name().c_str());
        setStatusTip (header().name().c_str());
        window.image_group->addAction (this);
        window.image_menu->addAction (this);
        connect (this, SIGNAL(scalingChanged()), &window, SLOT(on_scaling_changed()));
      }




            
      size_t Image::guess_colourmap () const 
      {
        std::string map = "Gray";
        if (header().datatype().is_complex()) 
          map = "Complex";
        else if (header().ndim() == 4) {
          if (header().dim(3) == 3)
            map = "RGB";
        }
        for (size_t n = 0; ColourMap::maps[n].name; ++n) 
          if (ColourMap::maps[n].name == map) 
            return n;
        return 0;
      }






      inline void Image::draw_vertices (const Point<float>* vertices)
      {
        if (!vertex_buffer || !vertex_array_object) {
          assert (!vertex_buffer);
          assert (!vertex_array_object);

          vertex_buffer.gen();
          vertex_array_object.gen();

          vertex_buffer.bind (GL_ARRAY_BUFFER);
          vertex_array_object.bind();

          glEnableVertexAttribArray (0);
          glVertexAttribPointer (0, 3, GL_FLOAT, GL_FALSE, 2*sizeof(Point<float>), (void*)0);

          glEnableVertexAttribArray (1);
          glVertexAttribPointer (1, 3, GL_FLOAT, GL_FALSE, 2*sizeof(Point<float>), (void*)(sizeof(Point<float>)));
        }
        else {
          vertex_buffer.bind (GL_ARRAY_BUFFER);
          vertex_array_object.bind();
        }

        glBufferData (GL_ARRAY_BUFFER, 8*sizeof(Point<float>), &vertices[0][0], GL_STREAM_DRAW);
        glDrawArrays (GL_QUADS, 0, 4);
      }





      void Image::render2D (Displayable::Shader& shader_program, const Projection& projection, int plane, int slice)
      {
        update_texture2D (plane, slice);

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


        start (shader_program);
        projection.set (shader_program);
        draw_vertices (vertices);
        stop (shader_program);
      }



      namespace {
        inline Point<> div (const Point<>& a, const Point<>& b) {
          return Point<> (a[0]/b[0], a[1]/b[1], a[2]/b[2]);
        }
      }

      void Image::render3D (Displayable::Shader& shader_program, const Projection& projection, float depth) 
      {
        update_texture3D();

        start (shader_program, windowing_scale_3D);
        projection.set (shader_program);

        Point<> vertices[8];

        vertices[0] = projection.screen_to_model (projection.x_position(), projection.y_position()+projection.height(), depth);
        vertices[2] = projection.screen_to_model (projection.x_position(), projection.y_position(), depth);
        vertices[4] = projection.screen_to_model (projection.x_position()+projection.width(), projection.y_position(), depth);
        vertices[6] = projection.screen_to_model (projection.x_position()+projection.width(), projection.y_position()+projection.height(), depth);

        Point<> dim (header().dim(0), header().dim(1), header().dim(2));
        vertices[1] = div (interp.scanner2voxel (vertices[0]) + Point<> (0.5, 0.5, 0.5), dim);
        vertices[3] = div (interp.scanner2voxel (vertices[2]) + Point<> (0.5, 0.5, 0.5), dim);
        vertices[5] = div (interp.scanner2voxel (vertices[4]) + Point<> (0.5, 0.5, 0.5), dim);
        vertices[7] = div (interp.scanner2voxel (vertices[6]) + Point<> (0.5, 0.5, 0.5), dim);

        draw_vertices (vertices);

        stop (shader_program);
      }



      void Image::update_texture2D (int plane, int slice)
      {
        if (!texture2D[plane]) { // allocate:
          texture2D[plane].gen (GL_TEXTURE_3D);
          texture2D[plane].bind();
        }
        else
          texture2D[plane].bind();
        glPixelStorei (GL_UNPACK_ALIGNMENT, 1);
        texture2D[plane].set_interp (interpolation);

        if (position[plane] == slice && volume_unchanged())
          return;

        position[plane] = slice;

        int x, y;
        get_axes (plane, x, y);
        ssize_t xdim = header().dim (x), ydim = header().dim (y);

        type = GL_FLOAT;
        Ptr<float,true> data;

        std::string cmap_name = ColourMap::maps[colourmap].name;

        if (cmap_name == "RGB") {

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
                  if (isfinite (mag)) {
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
        else if (cmap_name == "Complex") {

          data = new float [2*xdim*ydim];
          format = GL_RG;
          internal_format = GL_RG32F;

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
                if (isfinite (mag)) 
                  if (mag > value_max) 
                    value_max = mag;
              }
            }
          }
          value_min = 0.0;

        }
        else {

          data = new float [xdim*ydim];
          format = GL_RED;
          internal_format = GL_R32F;

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
                if (isfinite (val.real())) {
                  if (val.real() < value_min) value_min = val.real();
                  if (val.real() > value_max) value_max = val.real();
                }
              }
            }
          }

        }

        if ((value_max - value_min) < 2.0*std::numeric_limits<float>::epsilon()) 
          value_min = value_max - 1.0;

        update_levels();

        if (isnan (display_midpoint) || isnan (display_range))
          reset_windowing();

        glTexImage3D (GL_TEXTURE_3D, 0, internal_format, xdim, ydim, 1, 0, format, type, data);
      }







      void Image::update_texture3D ()
      {
        if (!texture3D) { // allocate:
          texture3D.gen (GL_TEXTURE_3D);
          texture3D.bind();
        }
        else 
          texture3D.bind();
        texture3D.set_interp (interpolation);

        if (volume_unchanged() && texture_mode_3D_unchanged)
          return;

        std::string cmap_name = ColourMap::maps[colourmap].name;

        if (cmap_name == "RGB") format = GL_RGB;
        else if (cmap_name == "Complex") format = GL_RG;
        else format = GL_RED;

        GLenum type;

        if (cmap_name == "Complex") {
          internal_format = GL_RG32F;
          type = GL_FLOAT;
        }
        else {

          switch (header().datatype() ()) {
            case DataType::Bit:
            case DataType::Int8:
              internal_format = ( format == GL_RED ? GL_R16F : GL_RGB16F );
              type = GL_BYTE;
              break;
            case DataType::UInt8:
              internal_format = ( format == GL_RED ? GL_R16F : GL_RGB16F );
              type = GL_UNSIGNED_BYTE;
              break;
            case DataType::UInt16LE:
            case DataType::UInt16BE:
              internal_format = ( format == GL_RED ? GL_R16F : GL_RGB16F );
              type = GL_UNSIGNED_SHORT;
              break;
            case DataType::Int16LE:
            case DataType::Int16BE:
              internal_format = ( format == GL_RED ? GL_R16F : GL_RGB16F );
              type = GL_SHORT;
              break;
            case DataType::UInt32LE:
            case DataType::UInt32BE:
              internal_format = ( format == GL_RED ? GL_R32F : GL_RGB32F );
              type = GL_UNSIGNED_INT;
              break;
            case DataType::Int32LE:
            case DataType::Int32BE:
              internal_format = ( format == GL_RED ? GL_R32F : GL_RGB32F );
              type = GL_INT;
              break;
            default:
              internal_format = ( format == GL_RED ? GL_R32F : GL_RGB32F );
              type = GL_FLOAT;
              break;
          }
        }


        texture_mode_3D_unchanged = true;

        glPixelStorei (GL_UNPACK_ALIGNMENT, 1);

        glTexImage3D (GL_TEXTURE_3D, 0, internal_format,
            header().dim(0), header().dim(1), header().dim(2),
            0, format, type, NULL);

        value_min = std::numeric_limits<float>::infinity();
        value_max = -std::numeric_limits<float>::infinity();

        if (format != GL_RG) {
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
            default:
              copy_texture_3D<float> (format);
              break;
          }
        }
        else 
          copy_texture_3D_complex();

        update_levels();

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
        return 1.0f;
      }


      template <typename ValueType>
        inline void Image::copy_texture_3D (GLenum format)
      {
        MR::Image::Buffer<ValueType> buffer_tmp (buffer);
        typename MR::Image::Buffer<ValueType>::voxel_type V (buffer_tmp);
        GLenum type = GLtype<ValueType>();
        int N = ( format == GL_RED ? 1 : 3 );
        Ptr<ValueType,true> data (new ValueType [N * V.dim(0) * V.dim(1)]);

        ProgressBar progress ("loading image data...", V.dim(2));

        for (size_t n = 3; n < V.ndim(); ++n) 
          V[n] = interp[n];

        for (V[2] = 0; V[2] < V.dim(2); ++V[2]) {

          if (format == GL_RED) {
            ValueType* p = data;

            for (V[1] = 0; V[1] < V.dim(1); ++V[1]) {
              for (V[0] = 0; V[0] < V.dim(0); ++V[0]) {
                ValueType val = *p = V.value();
                if (isfinite (val)) {
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
                  if (isfinite (val)) {
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
              V.dim(0), V.dim(1), 1,
              format, type, data);

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
              if (isfinite (mag)) {
                if (mag < value_min) value_min = mag;
                if (mag > value_max) value_max = mag;
              }
            }
          }

          glTexSubImage3D (GL_TEXTURE_3D, 0,
              0, 0, V[2],
              V.dim (0), V.dim (1), 1,
              GL_RG, GL_FLOAT, data);
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


