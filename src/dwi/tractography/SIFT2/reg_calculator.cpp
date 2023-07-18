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

#include <mutex>

#include "dwi/tractography/SIFT2/reg_calculator.h"
#include "dwi/tractography/SIFT2/tckfactor.h"

#include "dwi/tractography/SIFT/track_contribution.h"



namespace MR {
  namespace DWI {
    namespace Tractography {
      namespace SIFT2 {




      RegularisationCalculator::RegularisationCalculator (TckFactor& tckfactor, double& cf_reg_tik, double& cf_reg_tv) :
        master (tckfactor),
        cf_reg_tik (cf_reg_tik),
        cf_reg_tv (cf_reg_tv),
        tikhonov_sum (0.0),
        tv_sum (0.0) { }



      RegularisationCalculator::~RegularisationCalculator()
      {
        std::lock_guard<std::mutex> lock (master.mutex);
        cf_reg_tik += tikhonov_sum;
        cf_reg_tv  += tv_sum;
      }



      bool RegularisationCalculator::operator() (const SIFT::TrackIndexRange& range)
      {
        for (SIFT::track_t track_index = range.first; track_index != range.second; ++track_index) {
          const double coefficient = master.coefficients[track_index];
          tikhonov_sum += Math::pow2 (coefficient);
          const SIFT::TrackContribution& this_contribution (*(master.contributions[track_index]));
          const double contribution_multiplier = 1.0 / this_contribution.get_total_contribution();
          double this_tv_sum = 0.0;
          for (size_t j = 0; j != this_contribution.dim(); ++j) {
            const Fixel& fixel (master.fixels[this_contribution[j].get_fixel_index()]);
            const double fixel_coeff_cost = SIFT2::tvreg (coefficient, fixel.get_mean_coeff());
            this_tv_sum += fixel.get_weight() * this_contribution[j].get_length() * contribution_multiplier * fixel_coeff_cost;
          }
          tv_sum += this_tv_sum;
        }
        return true;
      }




      }
    }
  }
}



