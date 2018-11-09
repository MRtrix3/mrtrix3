/*
 * Copyright (c) 2008-2018 the MRtrix3 contributors.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, you can obtain one at http://mozilla.org/MPL/2.0/
 *
 * MRtrix3 is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * For more details, see http://www.mrtrix.org/
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
        MRView::GrabContext context;
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


