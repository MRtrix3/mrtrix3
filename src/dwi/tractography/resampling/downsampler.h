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

#ifndef __dwi_tractography_resampling_downsampler_h__
#define __dwi_tractography_resampling_downsampler_h__


#include "dwi/tractography/tracking/generated_track.h"
#include "dwi/tractography/resampling/resampling.h"


namespace MR {
  namespace DWI {
    namespace Tractography {
      namespace Resampling {



        class Downsampler : public BaseCRTP<Downsampler>
        { MEMALIGN(Downsampler)

          public:
            Downsampler () : ratio (0) { }
            Downsampler (const size_t downsample_ratio) : ratio (downsample_ratio) { }

            bool operator() (const Streamline<>&, Streamline<>&) const override;
            bool valid() const override { return (ratio >= 1); }

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



