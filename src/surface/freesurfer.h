/* Copyright (c) 2008-2017 the MRtrix3 contributors.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, you can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * MRtrix is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * For more details, see http://www.mrtrix.org/.
 */


#ifndef __surface_freesurfer_h__
#define __surface_freesurfer_h__

#include <stdint.h>
#include <fstream>

#include "raw.h"

#include "connectome/lut.h"

#include "surface/scalar.h"
#include "surface/types.h"

namespace MR
{
  namespace Surface
  {
    namespace FreeSurfer
    {


      constexpr int32_t triangle_file_magic_number = 16777214;
      constexpr int32_t quad_file_magic_number = 16777215;

      constexpr int32_t new_curv_file_magic_number = 16777215;


      inline int32_t get_int24_BE (std::ifstream& stream)
      {
        uint8_t bytes[3];
        for (size_t i = 0; i != 3; ++i)
          stream.read (reinterpret_cast<char*>(bytes+i), 1);
        return (int32_t(bytes[0]) << 16) | (int32_t(bytes[1]) << 8) | int32_t(bytes[2]);
      }



      template <typename T>
      inline T get_BE (std::ifstream& stream)
      {
        T temp;
        stream.read (reinterpret_cast<char*>(&temp), sizeof(T));
        return Raw::fetch_BE<T> (&temp);
      }



      void read_annot (const std::string&, label_vector_type&, Connectome::LUT&);
      void read_label (const std::string&, VertexList&, Scalar&);



    }
  }
}

#endif

