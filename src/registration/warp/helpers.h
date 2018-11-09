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


#ifndef __registration_warp_utils_h__
#define __registration_warp_utils_h__

namespace MR
{
  namespace Registration
  {

    namespace Warp
    {

      template <class HeaderType>
      inline void check_warp (const HeaderType& warp_header)
      {
        if (warp_header.ndim() != 4)
          throw Exception ("input warp is not a 4D image");
        if (warp_header.size(3) != 3)
          throw Exception ("input warp should have 3 volumes in the 4th dimension");
      }

      template <class HeaderType>
      inline void check_warp_full (const HeaderType& warp_header)
      {
        if (warp_header.ndim() != 5)
          throw Exception ("the input warp image must be a 5D file.");
        if (warp_header.size(3) != 3)
          throw Exception ("the input warp image must have 3 volumes (x,y,z) in the 4th dimension.");
        if (warp_header.size(4) != 4)
          throw Exception ("the input warp image must have 4 volumes in the 5th dimension.");
      }

      template <class InputWarpType>
      transform_type parse_linear_transform (InputWarpType& input_warps, std::string name) {
        transform_type linear;
        const auto it = input_warps.keyval().find (name);
        if (it != input_warps.keyval().end()) {
          const auto lines = split_lines (it->second);
          if (lines.size() != 3)
            throw Exception ("linear transform in initialisation syn warps image header does not contain 3 rows");
          for (size_t row = 0; row < 3; ++row) {
            const auto values = split (lines[row], " ", true);
            if (values.size() != 4)
              throw Exception ("linear transform in initialisation syn warps image header does not contain 4 columns");
            for (size_t col = 0; col < 4; ++col)
              linear (row, col) = std::stod (values[col]);
          }
        } else {
          throw Exception ("no linear transform found in initialisation syn warps image header");
        }

        return linear;
      }

    }
  }
}

#endif
