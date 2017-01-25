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


#ifndef __dwi_tractography_resampling_downsampler_h__
#define __dwi_tractography_resampling_downsampler_h__


#include <vector>

#include "dwi/tractography/resampling/resampling.h"


namespace MR {
  namespace DWI {
    namespace Tractography {
      namespace Resampling {



        class Downsampler : public Base
        { MEMALIGN(Downsampler)

          public:
            Downsampler () : ratio (1) { }
            Downsampler (const size_t downsample_ratio) : ratio (downsample_ratio) { }

            bool operator() (vector<Eigen::Vector3f>&) const override;
            bool valid() const override { return (ratio > 1); }

            // This version guarantees that the seed point is retained, and
            //   updates the index of the seed point appropriately
            bool operator() (Tracking::GeneratedTrack&) const;

            size_t get_ratio() const { return ratio; }
            void set_ratio (const size_t i) { ratio = i; }

          private:
            size_t ratio;

        };




      }
    }
  }
}

#endif



