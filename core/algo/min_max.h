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

#ifndef __image_min_max_h__
#define __image_min_max_h__

#include "algo/threaded_loop.h"

namespace MR
{

    //! \cond skip
    namespace {
      template <class ImageType>
      class __MinMax { NOMEMALIGN
        public:
          using value_type = typename ImageType::value_type;

          __MinMax (value_type& overall_min, value_type& overall_max) :
            overall_min (overall_min), overall_max (overall_max),
            min (std::numeric_limits<value_type>::infinity()),
            max (-std::numeric_limits<value_type>::infinity()) {
              overall_min = min;
              overall_max = max;
            }
          ~__MinMax () {
            std::lock_guard<std::mutex> lock (mutex);
            overall_min = std::min (overall_min, min);
            overall_max = std::max (overall_max, max);
          }

          void operator() (ImageType& vox) {
            value_type val = vox.value();
            if (std::isfinite (val)) {
              if (val < min) min = val;
              if (val > max) max = val;
            }
          }

          template <class MaskType>
          void operator() (ImageType& vox, MaskType& mask) {
            if (mask.value()) {
              value_type val = vox.value();
              if (std::isfinite (val)) {
                if (val < min) min = val;
                if (val > max) max = val;
              }
            }
          }

          value_type& overall_min;
          value_type& overall_max;
          value_type min, max;

          static std::mutex mutex;
      };
      template <class ImageType> std::mutex __MinMax<ImageType>::mutex;
    }
    //! \endcond

    template <class ImageType>
    inline void min_max (
        ImageType& in,
        typename ImageType::value_type& min,
        typename ImageType::value_type& max,
        size_t from_axis = 0,
        size_t to_axis = std::numeric_limits<size_t>::max())
    {
      ThreadedLoop ("finding min/max of \"" + shorten (in.name()) + "\"", in, from_axis, to_axis)
        .run (__MinMax<ImageType> (min, max), in);
    }

    template <class ImageType, class MaskType>
    inline void min_max (
        ImageType& in,
        MaskType& mask,
        typename ImageType::value_type& min,
        typename ImageType::value_type& max,
        size_t from_axis = 0,
        size_t to_axis = std::numeric_limits<size_t>::max())
    {
      ThreadedLoop ("finding min/max of \"" + shorten (in.name()) + "\"", in, from_axis, to_axis)
        .run (__MinMax<ImageType> (min, max), in, mask);
    }

}

#endif
