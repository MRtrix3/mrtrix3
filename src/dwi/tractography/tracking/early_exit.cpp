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


#include "dwi/tractography/tracking/early_exit.h"


namespace MR
{
  namespace DWI
  {
    namespace Tractography
    {
      namespace Tracking
      {



        bool EarlyExit::operator() (const Writer<>& writer)
        {
          if (++counter != next_test)
            return false;

          const size_t num_attempts = writer.total_count;
          const size_t num_tracks = writer.count;
          if ((num_attempts / default_type(max_num_attempts) > TCKGEN_EARLY_EXIT_STOP_TESTING_PERCENTAGE) ||
              (num_tracks   / default_type(max_num_tracks)   > TCKGEN_EARLY_EXIT_STOP_TESTING_PERCENTAGE)) {
            DEBUG ("tckgen early exit: No longer testing (tracking progressed beyond " + str<int>(std::round(100.0*TCKGEN_EARLY_EXIT_STOP_TESTING_PERCENTAGE)) + "%)");
            next_test = 0;
            return false;
          }

          const Eigen::Array<default_type, 2, 1> a ( { default_type(num_attempts-num_tracks), default_type(num_attempts-num_tracks) });
          const Eigen::Array<default_type, 2, 1> b ( { default_type(num_tracks+1), default_type(num_tracks+1) });
          const Eigen::Array<default_type, 2, 1> x ( { 1.0-(max_num_tracks/default_type(max_num_attempts)), 1.0 });
          const auto incomplete_betas = Eigen::betainc (a, b, x);
          const default_type conditional = incomplete_betas[0] / incomplete_betas[1];

          DEBUG ("tckgen early exit: Target " + str(max_num_tracks) + "/" + str(max_num_attempts) + " (" + str(max_num_tracks/default_type(max_num_attempts)) + "), current " + str(num_tracks) + "/" + str(num_attempts) + " (" + str(num_tracks/default_type(num_attempts)) + "), conditional probability " + str(conditional));
          next_test *= 2;
          return (conditional < TCKGEN_EARLY_EXIT_PROB_THRESHOLD);
        }



      }
    }
  }
}

