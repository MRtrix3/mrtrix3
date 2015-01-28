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
        Volume (image_header),
        buffer (image_header),
        interp (buffer),        
        position (image_header.ndim())
      {
        position[0] = position[1] = position[2] = std::numeric_limits<ssize_t>::min();
        set_colourmap (guess_colourmap ());
      }



      Image::Image (Window& window, const MR::Image::Header& image_header) :
        Volume (window, image_header),
        buffer (image_header),
        interp (buffer),        
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





      void Image::render2D (Displayable::Shader& shader_program, const Projection& projection, int plane, int slice)
      {
        update_texture2D (plane, slice);

        int x, y;
        get_axes (plane, x, y);
        float xdim = header().dim (x)-0.5, ydim = header().dim (y)-0.5;

        Point<> p, q;
        p[plane] = slice;

        p[x] = -0.5;
        p[y] = -0.5;
        vertices[0] = _transform.voxel2scanner (p);
        vertices[1].set (0.0, 0.0, 0.0);

        p[x] = -0.5;
        p[y] = ydim;
        vertices[2] = _transform.voxel2scanner (p);
        vertices[3].set (0.0, 1.0, 0.0);

        p[x] = xdim;
        p[y] = ydim;
        vertices[4] = _transform.voxel2scanner (p);
        vertices[5].set (1.0, 1.0, 0.0);

        p[x] = xdim;
        p[y] = -0.5;
        vertices[6] = _transform.voxel2scanner (p);
        vertices[7].set (1.0, 0.0, 0.0);


        start (shader_program);
        projection.set (shader_program);
        draw_vertices ();
        stop (shader_program);
      }



      void Image::render3D (Displayable::Shader& shader_program, const Projection& projection, float depth) 
      {
        update_texture3D();
        Volume::render (shader_program, projection, depth);
      }





      void Image::update_texture2D (int plane, int slice)
      {
        if (!texture2D[plane]) { // allocate:
          texture2D[plane].gen (gl::TEXTURE_3D);
          texture2D[plane].bind();
        }
        else
          texture2D[plane].bind();
        gl::PixelStorei (gl::UNPACK_ALIGNMENT, 1);
        texture2D[plane].set_interp (interpolation);

        if (position[plane] == slice && volume_unchanged())
          return;

        position[plane] = slice;

        int x, y;
        get_axes (plane, x, y);
        ssize_t xdim = header().dim (x), ydim = header().dim (y);

        type = gl::FLOAT;
        std::vector<float> data;
        auto& vox (voxel());

        std::string cmap_name = ColourMap::maps[colourmap].name;

        if (cmap_name == "RGB") {

          data.resize (3*xdim*ydim, 0.0f);
          format = gl::RGB;
          internal_format = gl::RGB32F;

          if (position[plane] >= 0 && position[plane] < header().dim (plane)) {
            // copy data:
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
                  float mag = std::abs (val.real());
                  data[3*(vox[x]+vox[y]*xdim) + n] = mag;
                  if (std::isfinite (mag)) {
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

          data.resize (2*xdim*ydim);
          format = gl::RG;
          internal_format = gl::RG32F;

          if (position[plane] < 0 || position[plane] >= header().dim (plane)) {
            for (auto& d : data) d = 0.0f;
          }
          else {
            // copy data:
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
                if (std::isfinite (mag)) 
                  if (mag > value_max) 
                    value_max = mag;
              }
            }
          }
          value_min = 0.0;

        }
        else {

          data.resize (xdim*ydim);
          format = gl::RED;
          internal_format = gl::R32F;

          if (position[plane] < 0 || position[plane] >= header().dim (plane)) {
            for (auto& d : data) d = 0.0f;
          }
          else {
            // copy data:
            vox[plane] = slice;
            value_min = std::numeric_limits<float>::infinity();
            value_max = -std::numeric_limits<float>::infinity();
            for (vox[y] = 0; vox[y] < ydim; ++vox[y]) {
              for (vox[x] = 0; vox[x] < xdim; ++vox[x]) {
                cfloat val = vox.value();
                data[vox[x]+vox[y]*xdim] = val.real();
                if (std::isfinite (val.real())) {
                  if (val.real() < value_min) value_min = val.real();
                  if (val.real() > value_max) value_max = val.real();
                }
              }
            }
          }

        }

        if ((value_max - value_min) < 2.0*std::numeric_limits<float>::epsilon()) 
          value_min = value_max - 1.0;

        set_min_max (value_min, value_max);

        gl::TexImage3D (gl::TEXTURE_3D, 0, internal_format, xdim, ydim, 1, 0, format, type, reinterpret_cast<void*> (&data[0]));
      }







      void Image::update_texture3D ()
      {
        if (volume_unchanged() && !texture_mode_changed) 
          return;
        bind();
        
        std::string cmap_name = ColourMap::maps[colourmap].name;

        if (cmap_name == "RGB") format = gl::RGB;
        else if (cmap_name == "Complex") format = gl::RG;
        else format = gl::RED;

        if (cmap_name == "Complex") {
          internal_format = gl::RG32F;
          type = gl::FLOAT;
        }
        else {

          switch (header().datatype() ()) {
            case DataType::Bit:
            case DataType::Int8:
              internal_format = ( format == gl::RED ? gl::R16F : gl::RGB16F );
              type = gl::BYTE;
              break;
            case DataType::UInt8:
              internal_format = ( format == gl::RED ? gl::R16F : gl::RGB16F );
              type = gl::UNSIGNED_BYTE;
              break;
            case DataType::UInt16LE:
            case DataType::UInt16BE:
              internal_format = ( format == gl::RED ? gl::R16F : gl::RGB16F );
              type = gl::UNSIGNED_SHORT;
              break;
            case DataType::Int16LE:
            case DataType::Int16BE:
              internal_format = ( format == gl::RED ? gl::R16F : gl::RGB16F );
              type = gl::SHORT;
              break;
            case DataType::UInt32LE:
            case DataType::UInt32BE:
              internal_format = ( format == gl::RED ? gl::R32F : gl::RGB32F );
              type = gl::UNSIGNED_INT;
              break;
            case DataType::Int32LE:
            case DataType::Int32BE:
              internal_format = ( format == gl::RED ? gl::R32F : gl::RGB32F );
              type = gl::INT;
              break;
            default:
              internal_format = ( format == gl::RED ? gl::R32F : gl::RGB32F );
              type = gl::FLOAT;
              break;
          }
        }

        allocate();
        texture_mode_changed = false;

        if (format != gl::RG) {
          switch (header().datatype() ()) {
            case DataType::Bit:
            case DataType::UInt8:
              copy_texture_3D<uint8_t> ();
              break;
            case DataType::Int8:
              copy_texture_3D<int8_t> ();
              break;
            case DataType::UInt16LE:
            case DataType::UInt16BE:
              copy_texture_3D<uint16_t> ();
              break;
            case DataType::Int16LE:
            case DataType::Int16BE:
              copy_texture_3D<int16_t> ();
              break;
            case DataType::UInt32LE:
            case DataType::UInt32BE:
              copy_texture_3D<uint32_t> ();
              break;
            case DataType::Int32LE:
            case DataType::Int32BE:
              copy_texture_3D<int32_t> ();
              break;
            default:
              copy_texture_3D<float> ();
              break;
          }
        }
        else 
          copy_texture_3D_complex();

        set_min_max (value_min, value_max);
      }

      // required to shut up clang's compiler warnings about std::abs() when
      // instantiating Image::copy_texture_3D() with unsigned types:
      template <typename ValueType> 
        inline ValueType abs_if_signed (ValueType x, typename std::enable_if<!std::is_unsigned<ValueType>::value>::type* = nullptr) { return std::abs(x); }

      template <typename ValueType> 
        inline ValueType abs_if_signed (ValueType x, typename std::enable_if<std::is_unsigned<ValueType>::value>::type* = nullptr) { return x; }


      template <typename ValueType>
        inline void Image::copy_texture_3D ()
        {
          MR::Image::Buffer<ValueType> buffer_tmp (buffer);
          auto V = buffer_tmp.voxel();
          int N = ( format == gl::RED ? 1 : 3 );
          std::vector<ValueType> data (N * V.dim(0) * V.dim(1));

          ProgressBar progress ("loading image data...", V.dim(2));

          for (size_t n = 3; n < V.ndim(); ++n) 
            V[n] = position[n];

        for (V[2] = 0; V[2] < V.dim(2); ++V[2]) {

          if (format == gl::RED) {
            auto p = data.begin();

            for (V[1] = 0; V[1] < V.dim(1); ++V[1]) {
              for (V[0] = 0; V[0] < V.dim(0); ++V[0]) {
                ValueType val = *p = V.value();
                if (std::isfinite (val)) {
                  if (val < value_min) value_min = val;
                  if (val > value_max) value_max = val;
                }
                ++p;
              }
            }

          }
          else {

            for (auto& d : data) d = 0.0f;

            for (size_t n = 0; n < 3; ++n) {
              if (V.ndim() > 3) {
                if (V.dim(3) > int(position[3] + n))
                  V[3] = position[3] + n;
                else break;
              }

              auto p = data.begin() + n;
              for (V[1] = 0; V[1] < V.dim (1); ++V[1]) {
                for (V[0] = 0; V[0] < V.dim (0); ++V[0]) {
                  ValueType val = *p = abs_if_signed (ValueType (V.value()));
                  if (std::isfinite (val)) {
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

          upload_data ({ { 0, 0, V[2] } }, { { V.dim(0), V.dim(1), 1 } }, reinterpret_cast<void*> (&data[0]));
          ++progress;
        }

      }



      inline void Image::copy_texture_3D_complex ()
      {
        auto V = buffer.voxel();
        std::vector<float> data (2 * V.dim (0) * V.dim (1));

        ProgressBar progress ("loading image data...", V.dim (2));

        for (size_t n = 3; n < V.ndim(); ++n) 
          V[n] = position[n];

        for (V[2] = 0; V[2] < V.dim (2); ++V[2]) {
          auto p = data.begin();

          for (V[1] = 0; V[1] < V.dim (1); ++V[1]) {
            for (V[0] = 0; V[0] < V.dim (0); ++V[0]) {
              cfloat val = V.value();
              *(p++) = val.real();
              *(p++) = val.imag();
              float mag = std::abs (val);
              if (std::isfinite (mag)) {
                if (mag < value_min) value_min = mag;
                if (mag > value_max) value_max = mag;
              }
            }
          }

          upload_data ({ { 0, 0, V[2] } }, { { V.dim(0), V.dim(1), 1 } }, reinterpret_cast<void*> (&data[0]));
          ++progress;
        }
      }



      inline bool Image::volume_unchanged ()
      {
        bool is_unchanged = true;
        for (size_t i = 3; i < buffer.ndim(); ++i) {
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


