/*
 * Copyright (c) 2008-2016 the MRtrix3 contributors
 * 
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/
 * 
 * MRtrix is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * 
 * For more details, see www.mrtrix.org
 * 
 */


#ifndef __dwi_tractography_sift2_fixel_updater_h__
#define __dwi_tractography_sift2_fixel_updater_h__


#include <vector>

#include "dwi/tractography/SIFT/track_index_range.h"
#include "dwi/tractography/SIFT/types.h"


namespace MR {
  namespace DWI {
    namespace Tractography {
      namespace SIFT2 {


      class TckFactor;


      class FixelUpdater
      { MEMALIGN(FixelUpdater)

        public:
          FixelUpdater (TckFactor&);
          ~FixelUpdater();

          bool operator() (const SIFT::TrackIndexRange& range);

        private:
          TckFactor& master;

          // Each thread needs a local copy of these
          std::vector<double> fixel_coeff_sums;
          std::vector<double> fixel_TDs;
          std::vector<SIFT::track_t> fixel_counts;

      };



      }
    }
  }
}



#endif

