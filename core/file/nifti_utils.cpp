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


#include "header.h"
#include "raw.h"
#include "file/config.h"
#include "file/nifti_utils.h"

namespace MR
{
  namespace File
  {
    namespace NIfTI
    {


      bool right_left_warning_issued = false;



      transform_type adjust_transform (const Header& H, vector<size_t>& axes)
      {
        Stride::List strides = Stride::get (H);
        strides.resize (3);
        axes = Stride::order (strides);
        bool flip[] = { strides[axes[0]] < 0, strides[axes[1]] < 0, strides[axes[2]] < 0 };

        if (axes[0] == 0 && axes[1] == 1 && axes[2] == 2 &&
            !flip[0] && !flip[1] && !flip[2])
          return H.transform();

        const auto& M_in = H.transform().matrix();
        transform_type out (H.transform());
        auto& M_out = out.matrix();

        for (size_t i = 0; i < 3; i++)
          M_out.col (i) = M_in.col (axes[i]);

        auto translation = M_out.col(3);
        for (size_t i = 0; i < 3; ++i) {
          if (flip[i]) {
            auto length = default_type (H.size (axes[i])-1) * H.spacing (axes[i]);
            auto axis = M_out.col(i);
            axis = -axis;
            translation -= length * axis;
          }
        }

        return out;
      }





      void check (Header& H, const bool is_analyse)
      {
        for (size_t i = 0; i < H.ndim(); ++i)
          if (H.size (i) < 1)
            H.size(i) = 1;

        // ensure first 3 axes correspond to spatial dimensions
        // while preserving original strides as much as possible
        ssize_t max_spatial_stride = 0;
        for (size_t n = 0; n < 3; ++n)
          if (std::abs(H.stride(n)) > max_spatial_stride)
            max_spatial_stride = std::abs(H.stride(n));
        for (size_t n = 3; n < H.ndim(); ++n)
          H.stride(n) += H.stride(n) > 0 ? max_spatial_stride : -max_spatial_stride;
        Stride::symbolise (H);

        // if .img, reset all strides to defaults, since it can't be assumed
        // that downstream software will be able to parse the NIfTI transform 
        if (is_analyse) {
          for (size_t i = 0; i < H.ndim(); ++i)
            H.stride(i) = i+1;
          bool analyse_left_to_right = File::Config::get_bool ("Analyse.LeftToRight", false);
          if (analyse_left_to_right)
            H.stride(0) = -H.stride (0);

          if (!right_left_warning_issued) {
            INFO ("assuming Analyse images are encoded " + std::string (analyse_left_to_right ? "left to right" : "right to left"));
            right_left_warning_issued = true;
          }
        }

        // by default, prevent output of bitwise data in NIfTI, since most 3rd
        // party software package can't handle them

        //CONF option: NIfTIAllowBitwise
        //CONF default: 0 (false)
        //CONF A boolean value to indicate whether bitwise storage of binary
        //CONF data is permitted (most 3rd party software packages don't
        //CONF support bitwise data). If false (the default), data will be
        //CONF stored using more widely supported unsigned 8-bit integers.
        if (H.datatype() == DataType::Bit) 
          if (!File::Config::get_bool ("NIfTIAllowBitwise", false))
            H.datatype() = DataType::UInt8;
      }






      size_t version (Header& H)
      {
        //CONF option: NIfTIAlwaysUseVer2
        //CONF default: 0 (false)
        //CONF A boolean value to indicate whether NIfTI images should
        //CONF always be written in the new NIfTI-2 format. If false,
        //CONF images will be written in the older NIfTI-1 format by
        //CONF default, with the exception being files where the number
        //CONF of voxels along any axis exceeds the maximum permissible
        //CONF in that format (32767), in which case the output file
        //CONF will automatically switch to the NIfTI-2 format.
        if (File::Config::get_bool ("NIfTIAlwaysUseVer2", false))
          return 2;

        for (size_t axis = 0; axis != H.ndim(); ++axis) {
          if (H.size(axis) > std::numeric_limits<int16_t>::max()) {
            INFO ("Forcing file \"" + H.name() + "\" to use NIfTI version 2 due to image dimensions");
            return 2;
          }
        }

        return 1;
      }



    }
  }
}

