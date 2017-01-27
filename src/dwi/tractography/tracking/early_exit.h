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


#ifndef __dwi_tractography_tracking_early_exit_h__
#define __dwi_tractography_tracking_early_exit_h__

#include <Eigen/Core>

#if EIGEN_VERSION_AT_LEAST(3,3,0)
#define TCKGEN_EARLY_EXIT_USE_FULL_BINOMIAL
#include <unsupported/Eigen/SpecialFunctions>
#define TCKGEN_EARLY_EXIT_PROB_THRESHOLD 0.001
#else
#define TCKGEN_EARLY_EXIT_PROB_THRESHOLD 1e-6
#define TCKGEN_EARLY_EXIT_ZVALUE -4.753408 // For a p-value of 1e-6; should be negative
#endif

#include "dwi/tractography/file.h"
#include "dwi/tractography/tracking/shared.h"


#define TCKGEN_EARLY_EXIT_STOP_TESTING_PERCENTAGE 0.20


namespace MR
{
  namespace DWI
  {
    namespace Tractography
    {
      namespace Tracking
      {


        class EarlyExit
        { NOMEMALIGN
          public:
            EarlyExit (const SharedBase& shared) :
              max_num_attempts (shared.max_num_attempts),
              max_num_tracks (shared.max_num_tracks),
              counter (0),
              next_test ((max_num_attempts && max_num_tracks) ? (10 * max_num_attempts / max_num_tracks) : 0) { }

            bool operator() (const Writer<>&);

          private:
            const size_t max_num_attempts, max_num_tracks;
            size_t counter, next_test;
        };



      }
    }
  }
}

#endif


