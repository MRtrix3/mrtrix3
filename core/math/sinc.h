/* Copyright (c) 2008-2017 the MRtrix3 contributors
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


#ifndef __math_sinc_h__
#define __math_sinc_h__

#include "math/math.h"

namespace MR
{
  namespace Math
  {

    template <typename T = float> class Sinc
    { NOMEMALIGN
      public:
        typedef T value_type;

        Sinc (const size_t w) :
          window_size (w),
          max_offset_from_kernel_centre ((w-1) / 2),
          indices (w),
          weights (w),
          current_pos (NAN)
        {
          assert (w % 2);
        }

        template <class ImageType>
        void set (const ImageType& image, const size_t axis, const value_type position) {

          if (position == current_pos)
            return;

          const int kernel_centre = std::round (position);
          value_type sum_weights = 0.0;

          for (size_t i = 0; i != window_size; ++i) {

            const int voxel = kernel_centre - max_offset_from_kernel_centre + i;
            if (voxel < 0)
              indices[i] = -voxel - 1;
            else if (voxel >= image.size (axis))
              indices[i] = (2 * int(image.size (axis))) - voxel - 1;
            else
              indices[i] = voxel;

            const value_type offset = position - (value_type)voxel;

            const value_type sinc   = offset ? std::sin (Math::pi * offset) / (Math::pi * offset) : 1.0;

            //const value_type hann_cos_term = Math::pi * offset / (value_type(max_offset_from_kernel_centre) + 0.5);
            //const value_type hann_factor   = (std::abs (hann_cos_term) < Math::pi) ? 0.5 * (1.0 + std::cos (hann_cos_term)) : 0.0;
            //const value_type this_weight   = hann_factor * sinc;

            const value_type lanczos_sinc_term = std::abs (Math::pi * offset / (double(max_offset_from_kernel_centre) + 0.5));
            value_type lanczos_factor = 0.0;
            if (lanczos_sinc_term < Math::pi) {
              if (lanczos_sinc_term)
                lanczos_factor = std::sin (lanczos_sinc_term) / lanczos_sinc_term;
              else
                lanczos_factor = 1.0;
            }
            const value_type this_weight = lanczos_factor * sinc;

            weights[i]  =  this_weight;
            sum_weights += this_weight;

          }

          const value_type normalisation = 1.0 / sum_weights;
          for (size_t i = 0; i != window_size; ++i)
            weights[i] *= normalisation;

          current_pos = position;

        }

        size_t index (const size_t i) const { return indices[i]; }

        template <class ImageType>
        value_type value (ImageType& image, const size_t axis) const {
          assert (current_pos != NAN);
          const size_t init_pos = image.index(axis);
          value_type sum = 0.0;
          for (size_t i = 0; i != window_size; ++i) {
            image.index(axis) = indices[i];
            sum += image.value() * weights[i];
          }
          image.index(axis) = init_pos;
          return sum;
        }

        template <class Cont>
        value_type value (Cont& data) const {
          assert (data.size() == window_size);
          assert (current_pos != NAN);
          value_type sum = 0.0;
          for (size_t i = 0; i != window_size; ++i)
            sum += data[i] * weights[i];
          return sum;
        }

        value_type value (value_type* data) const {
          assert (current_pos != NAN);
          value_type sum = 0.0;
          for (size_t i = 0; i != window_size; ++i)
            sum += data[i] * weights[i];
          return sum;
        }

      private:
        const size_t window_size, max_offset_from_kernel_centre;
        vector<size_t> indices;
        vector<value_type> weights;
        value_type  current_pos;

    };

  }
}

#endif
