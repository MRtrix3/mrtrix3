/* Copyright (c) 2008-2019 the MRtrix3 contributors.
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

#ifndef __image_filter_kernels_h__
#define __image_filter_kernels_h__

#include <array>

#include "adapter/base.h"

namespace MR
{
  namespace Filter
  {
    namespace Kernels
    {


      // TODO More advanced kernel class that internally stores kernel half-widths
      using kernel_type = Eigen::Matrix<default_type, Eigen::Dynamic, 1>;
      using kernel_triplet = std::array<kernel_type, 3>;

      kernel_type identity (const size_t size);

      kernel_type boxblur (const size_t size);
      kernel_type boxblur (const vector<int>& size);

      kernel_type laplacian3d();
      kernel_type unsharp_mask (const default_type force);

      kernel_triplet sobel();
      kernel_triplet sobel_feldman();

      kernel_triplet farid (const size_t order, const size_t size);

      extern const kernel_triplet farid_1st_3x3x3;
      extern const kernel_triplet farid_1st_5x5x5;


    }
  }
}

#endif
