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
          typedef typename ImageType::value_type value_type;

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
