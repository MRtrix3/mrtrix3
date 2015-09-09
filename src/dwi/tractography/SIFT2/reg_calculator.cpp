/*
    Copyright 2011 Brain Research Institute, Melbourne, Australia

    Written by Robert Smith, 2013.

    This file is part of MRtrix.

    MRtrix is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    MRtrix is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with MRtrix.  If not, see <http://www.gnu.org/licenses/>.

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
          const float coefficient = master.coefficients[track_index];
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



