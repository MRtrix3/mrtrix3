/* Copyright (c) 2008-2023 the MRtrix3 contributors.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Covered Software is provided under this License on an "as is"
 * basis, without warranty of any kind, either expressed, implied, or
 * statutory, including, without limitation, warranties that the
 * Covered Software is free of defects, merchantable, fit for a
 * particular purpose or non-infringing.
 * See the Mozilla Public License v. 2.0 for more details.
 *
 * For more details, see http://www.mrtrix.org/.
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
        GL::Context::Grab context;
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
            case gl::BYTE: _scale_factor = float (std::numeric_limits<int8_t>::max()); break;
            case gl::UNSIGNED_BYTE: _scale_factor = float (std::numeric_limits<uint8_t>::max()); break;
            case gl::SHORT: _scale_factor = float (std::numeric_limits<int16_t>::max()); break;
            case gl::UNSIGNED_SHORT: _scale_factor = float (std::numeric_limits<uint16_t>::max()); break;
            case gl::INT: _scale_factor = float (std::numeric_limits<int32_t>::max()); break;
            case gl::UNSIGNED_INT: _scale_factor = float (std::numeric_limits<uint32_t>::max()); break;
            default: _scale_factor = 1.0f;
        }
      }



    }
  }
}


