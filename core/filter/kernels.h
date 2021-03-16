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

  class Header;

  namespace Filter
  {
    namespace Kernels
    {


      class kernel_type : public Eigen::Matrix<default_type, Eigen::Dynamic, 1>
      { MEMALIGN(kernel_type)
        public:
          using base_type = Eigen::Matrix<default_type, Eigen::Dynamic, 1>;

          kernel_type() :
              fullwidths ({0, 0, 0}),
              halfwidths ({0, 0, 0}) { }

          kernel_type (const size_t size) :
              base_type (Math::pow3 (size))
          {
            assert (size%2);
            fullwidths[0] = fullwidths[1] = fullwidths[2] = size;
            halfwidths[0] = halfwidths[1] = halfwidths[2] = (size-1) / 2;
          }

          kernel_type (const vector<int>& sizes) :
              base_type (sizes[0] * sizes[1] * sizes[2])
          {
            assert (sizes.size() == 3);
            for (ssize_t axis = 0; axis != 3; ++axis) {
              assert (sizes[axis] % 2);
              fullwidths[axis] = sizes[axis];
              halfwidths[axis] = (sizes[axis]-1) / 2;
            }
          }

          kernel_type (const base_type& data) :
              base_type (data)
          {
            const ssize_t size = ssize_t (round (std::cbrt (data.size())));
            assert (Math::pow3 (size) == data.size());
            fullwidths[0] = fullwidths[1] = fullwidths[2] = size;
            halfwidths[0] = halfwidths[1] = halfwidths[2] = (size-1) / 2;
          }

          void resize (const ssize_t) = delete;
          using base_type::size;
          ssize_t size (const ssize_t axis) const { assert (axis >= 0 && axis < 3); return fullwidths[axis]; }
          const std::array<ssize_t, 3> sizes() const { return fullwidths; }
          ssize_t halfsize (const ssize_t axis) const { assert (axis >= 0 && axis < 3); return halfwidths[axis]; }

        private:
          std::array<ssize_t, 3> fullwidths, halfwidths;
      };



      //using kernel_type = Eigen::Matrix<default_type, Eigen::Dynamic, 1>;
      using kernel_triplet = std::array<kernel_type, 3>;

      kernel_type identity (const ssize_t size);

      kernel_type boxblur (const ssize_t size);
      kernel_type boxblur (const vector<int>& size);

      kernel_type gaussian (const Header& header, const default_type fwhm, const default_type radius);

      kernel_type laplacian3d();

      kernel_type radialblur (const Header& header, const default_type radius);

      kernel_type sharpen (const default_type strength);

      kernel_type unsharp_mask (const Header& header, const default_type smooth_fwhm, const default_type sharpen_strength);


      kernel_triplet sobel();
      kernel_triplet sobel_feldman();

      kernel_triplet farid (const size_t order, const size_t size);



    }
  }
}

#endif
