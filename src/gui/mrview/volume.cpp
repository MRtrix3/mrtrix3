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

#include "gui/mrview/volume.h"

#include "gui/mrview/window.h"


namespace MR
{
  namespace GUI
  {
    namespace MRView
    {


      Volume::~Volume() {
        Window::GrabContext context;
        _texture.clear();
        vertex_buffer.clear();
        vertex_array_object.clear();
      }

      void Volume::allocate ()
      {
        gl::PixelStorei (gl::UNPACK_ALIGNMENT, 1);

        gl::TexImage3D (gl::TEXTURE_3D, 0, internal_format,
            _header.size(0), _header.size(1), _header.size(2),
            0, format, type, NULL);

        value_min = std::numeric_limits<float>::infinity();
        value_max = -std::numeric_limits<float>::infinity();

        switch (type) {
            case gl::BYTE: _scale_factor = std::numeric_limits<int8_t>::max(); break;
            case gl::UNSIGNED_BYTE: _scale_factor = std::numeric_limits<uint8_t>::max(); break;
            case gl::SHORT: _scale_factor = std::numeric_limits<int16_t>::max(); break;
            case gl::UNSIGNED_SHORT: _scale_factor = std::numeric_limits<uint16_t>::max(); break;
            case gl::INT: _scale_factor = std::numeric_limits<int32_t>::max(); break;
            case gl::UNSIGNED_INT: _scale_factor = std::numeric_limits<uint32_t>::max(); break;
            default: _scale_factor = 1.0f;
        }
      }


    }
  }
}


