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

          __MinMax (value_type& overal_min, value_type& overal_max) :
            overal_min (overal_min), overal_max (overal_max),
            min (std::numeric_limits<value_type>::infinity()), 
            max (-std::numeric_limits<value_type>::infinity()) { 
              overal_min = min;
              overal_max = max;
            }
          ~__MinMax () {
            std::lock_guard<std::mutex> lock (mutex);
            overal_min = std::min (overal_min, min);
            overal_max = std::max (overal_max, max);
          }

          void operator() (ImageType& vox) {
            value_type val = vox.value();
            if (std::isfinite (val)) {
              if (val < min) min = val;
              if (val > max) max = val;
            }
          }

          value_type& overal_min;
          value_type& overal_max;
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
      ThreadedLoop ("finding min/max of \"" + shorten (in.name()) + "\"", in)
        .run (__MinMax<ImageType> (min, max), in);
    }

}

#endif
