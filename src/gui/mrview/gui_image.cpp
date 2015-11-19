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

#include "gui/mrview/gui_image.h"

#include "header.h"
#include "progressbar.h"
#include "gui/mrview/window.h"
#include "gui/projection.h"



namespace MR
{
  namespace GUI
  {
    namespace MRView
    {



      ImageBase::ImageBase (MR::Header&& H) :
          Volume (std::move (H)),
          tex_positions (header().ndim(), 0)
      {
        tex_positions[0] = tex_positions[1] = tex_positions[2] = -1;
      }

      ImageBase::~ImageBase()
      {
        MRView::GrabContext context;
        for (size_t axis = 0; axis != 3; ++axis) {
          if (texture2D[axis])
            texture2D[axis].clear();
        }
      }




      void ImageBase::render2D (Displayable::Shader& shader_program, const Projection& projection, const int plane, const int slice)
      {
        update_texture2D (plane, slice);

        int x, y;
        get_axes (plane, x, y);
        float xsize = header().size(x)-0.5, ysize = header().size(y)-0.5;

        Eigen::Vector3f p;
        p[plane] = slice;

        p[x] = -0.5;
        p[y] = -0.5;
        vertices[0].noalias() = _transform.voxel2scanner.cast<float>() * p;
        vertices[1] = { 0.0f, 0.0f, 0.0f };

        p[x] = -0.5;
        p[y] = ysize;
        vertices[2].noalias() = _transform.voxel2scanner.cast<float>() * p;
        vertices[3] = { 0.0f, 1.0f, 0.0f };

        p[x] = xsize;
        p[y] = ysize;
        vertices[4].noalias() = _transform.voxel2scanner.cast<float>() * p;
        vertices[5] = { 1.0f, 1.0f, 0.0f };

        p[x] = xsize;
        p[y] = -0.5;
        vertices[6].noalias() = _transform.voxel2scanner.cast<float>() * p;
        vertices[7] = { 1.0f, 0.0f, 0.0f };

        start (shader_program);
        projection.set (shader_program);
        draw_vertices ();
        stop (shader_program);
      }

      void ImageBase::render3D (Displayable::Shader& shader_program, const Projection& projection, const float depth)
      {
        update_texture3D();
        Volume::render (shader_program, projection, depth);
      }

      void ImageBase::get_axes (const int plane, int& x, int& y) const {
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










      Image::Image (MR::Header&& image_header) :
          ImageBase (std::move (image_header)),
          image (header().get_image<cfloat>()),
          linear_interp (image),
          nearest_interp (image)
      {
        set_colourmap (guess_colourmap());
        const std::map<std::string, std::string>::const_iterator i = header().keyval().find ("comments");
        if (i != header().keyval().end())
          _comments = split_lines (i->second);
      }



      size_t Image::guess_colourmap() const
      {
        std::string map = "Gray";
        if (header().datatype().is_complex())
          map = "Complex";
        else if (header().ndim() == 4) {
          if (header().size(3) == 3)
            map = "RGB";
        }
        for (size_t n = 0; ColourMap::maps[n].name; ++n)
          if (ColourMap::maps[n].name == map)
            return n;
        return 0;
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

        if (tex_positions[plane] == slice && volume_unchanged () && format_unchanged ())
          return;

        tex_positions[plane] = slice;

        int x, y;
        get_axes (plane, x, y);
        const ssize_t xsize = header().size (x), ysize = header().size (y);

        type = gl::FLOAT;
        std::vector<float> data;

        std::string cmap_name = ColourMap::maps[colourmap].name;

        if (cmap_name == "RGB") {

          data.resize (3*xsize*ysize, 0.0f);
          format = gl::RGB;
          internal_format = gl::RGB32F;

          if (tex_positions[plane] >= 0 && tex_positions[plane] < header().size (plane)) {
            // copy data:
            image.index (plane) = slice;
            value_min = std::numeric_limits<float>::infinity();
            value_max = -std::numeric_limits<float>::infinity();

            for (size_t n = 0; n < 3; ++n) {
              if (image.ndim() > 3) {
                if (image.size (3) > int(tex_positions[3] + n))
                  image.index (3) = tex_positions[3] + n;
                else break;
              }
              for (image.index (y) = 0; image.index (y) < ysize; ++image.index (y)) {
                for (image.index (x) = 0; image.index (x) < xsize; ++image.index (x)) {
                  cfloat val = image.value();
                  float mag = std::abs (val.real());
                  data[3*(image.index(x)+image.index(y)*xsize) + n] = mag;
                  if (std::isfinite (mag)) {
                    if (mag < value_min) value_min = mag;
                    if (mag > value_max) value_max = mag;
                  }
                }
              }

              if (image.ndim() <= 3)
                break;
            }
            if (image.ndim() > 3)
              image.index (3) = tex_positions[3];
          }

        }
        else if (cmap_name == "Complex") {

          data.resize (2*xsize*ysize);
          format = gl::RG;
          internal_format = gl::RG32F;

          if (tex_positions[plane] < 0 || tex_positions[plane] >= header().size (plane)) {
            for (auto& d : data) d = 0.0f;
          }
          else {
            // copy data:
            image.index (plane) = slice;
            value_min = std::numeric_limits<float>::infinity();
            value_max = -std::numeric_limits<float>::infinity();
            for (image.index (y) = 0; image.index (y) < ysize; ++image.index (y)) {
              for (image.index (x) = 0; image.index (x) < xsize; ++image.index (x)) {
                cfloat val = image.value();
                size_t idx = 2*(image.index(x)+image.index(y)*xsize);
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

          data.resize (xsize*ysize);
          format = gl::RED;
          internal_format = gl::R32F;

          if (tex_positions[plane] < 0 || tex_positions[plane] >= header().size (plane)) {
            for (auto& d : data) d = 0.0f;
          }
          else {
            // copy data:
            image.index (plane) = slice;
            value_min = std::numeric_limits<float>::infinity();
            value_max = -std::numeric_limits<float>::infinity();
            for (image.index(y) = 0; image.index(y) < ysize; ++image.index(y)) {
              for (image.index(x) = 0; image.index(x) < xsize; ++image.index(x)) {
                cfloat val = image.value();
                data[image.index(x)+image.index(y)*xsize] = val.real();
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

        gl::TexImage3D (gl::TEXTURE_3D, 0, internal_format, xsize, ysize, 1, 0, format, type, reinterpret_cast<void*> (&data[0]));
      }







      void Image::update_texture3D ()
      {
        // Binding also guarantees texture interpolation is updated
        bind();

        if (volume_unchanged() && !texture_mode_changed)
          return;
        
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
            case DataType::UInt8:
              internal_format = ( format == gl::RED ? gl::R16F : gl::RGB16F );
              type = gl::UNSIGNED_BYTE;
              break;
            case DataType::Int8:
              internal_format = ( format == gl::RED ? gl::R16F : gl::RGB16F );
              type = gl::BYTE;
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
          auto V (image);
          const size_t N = ( format == gl::RED ? 1 : 3 );
          std::vector<ValueType> data (N * V.size(0) * V.size(1));

          ProgressBar progress ("loading image data...", V.size(2));

          for (size_t n = 3; n < V.ndim(); ++n) 
            V.index (n) = tex_positions[n];

          for (V.index(2) = 0; V.index(2) < V.size(2); ++V.index(2)) {

            if (format == gl::RED) {
              auto p = data.begin();

              for (V.index(1) = 0; V.index(1) < V.size(1); ++V.index(1)) {
                for (V.index(0) = 0; V.index(0) < V.size(0); ++V.index(0)) {
                  const float value = cfloat(V.value()).real();
                  const ValueType val = *p = (std::is_integral<ValueType>::value ? std::round (value) : value);
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
                  if (V.size(3) > int(tex_positions[3] + n))
                    V.index (3) = tex_positions[3] + n;
                  else break;
                }

                auto p = data.begin() + n;
                for (V.index(1) = 0; V.index(1) < V.size(1); ++V.index(1)) {
                  for (V.index(0) = 0; V.index(0) < V.size(0); ++V.index(0)) {
                    const float value = cfloat(V.value()).real();
                    const ValueType val = *p = (std::is_integral<ValueType>::value ? std::round (value) : value);
                    if (std::isfinite (val)) {
                      if (val < value_min) value_min = val;
                      if (val > value_max) value_max = val;
                    }
#ifndef NDEBUG
                    if (std::distance (p, data.end()) > 3)
#endif
                    p += 3;
                  }
                }

                if (V.ndim() <= 3) 
                  break;
              }
              if (V.ndim() > 3) 
                V.index (3) = tex_positions[3];

            }

            upload_data ({ { 0, 0, V.index(2) } }, { { V.size(0), V.size(1), 1 } }, reinterpret_cast<void*> (&data[0]));
            ++progress;
          }

        }



      inline void Image::copy_texture_3D_complex ()
      {
        std::vector<float> data (2 * image.size (0) * image.size (1));

        ProgressBar progress ("loading image data...", image.size (2));

        for (size_t n = 3; n < image.ndim(); ++n)
          image.index (n) = tex_positions[n];

        for (image.index(2) = 0; image.index(2) < image.size(2); ++image.index(2)) {
          auto p = data.begin();

          for (image.index(1) = 0; image.index(1) < image.size(1); ++image.index(1)) {
            for (image.index(0) = 0; image.index(0) < image.size(0); ++image.index(0)) {
              cfloat val = image.value();
              *(p++) = val.real();
              *(p++) = val.imag();
              float mag = std::abs (val);
              if (std::isfinite (mag)) {
                if (mag < value_min) value_min = mag;
                if (mag > value_max) value_max = mag;
              }
            }
          }

          upload_data ({ { 0, 0, image.index(2) } }, { { image.size(0), image.size(1), 1 } }, reinterpret_cast<void*> (&data[0]));
          ++progress;
        }
      }




      cfloat Image::trilinear_value (const Eigen::Vector3f& scanner_point) const {
        if (linear_interp.scanner (scanner_point))
          return cfloat(NAN, NAN);
        for (size_t n = 3; n < image.ndim(); ++n)
          linear_interp.index (n) = image.index (n);
        return linear_interp.value();
      }
      cfloat Image::nearest_neighbour_value (const Eigen::Vector3f& scanner_point) const {
        if (nearest_interp.scanner (scanner_point))
          return cfloat(NAN, NAN);
        for (size_t n = 3; n < image.ndim(); ++n)
          nearest_interp.index (n) = image.index (n);
        return nearest_interp.value();
      }




      inline bool Image::volume_unchanged ()
      {
        bool is_unchanged = true;
        for (size_t i = 3; i < image.ndim(); ++i) {
          if (image.index (i) != tex_positions[i]) {
            is_unchanged = false;
            tex_positions[i] = image.index (i);
          }
        }

        if (!is_unchanged) 
          tex_positions[0] = tex_positions[1] = tex_positions[2] = -1;

        return is_unchanged;
      }


      inline bool Image::format_unchanged ()
      {
        std::string cmap_name = ColourMap::maps[colourmap].name;

        if (cmap_name == "RGB" && format != gl::RGB)
          return false;
        else if (cmap_name == "Complex" && format != gl::RG)
          return false;
        else if (format != gl::RED)
          return false;

        return true;
      }


    }
  }
}


