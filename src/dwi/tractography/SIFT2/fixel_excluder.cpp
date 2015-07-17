/*
    Copyright 2011 Brain Research Institute, Melbourne, Australia

    Written by Robert Smith, 2014.

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


#include "dwi/tractography/SIFT2/fixel_excluder.h"
#include "dwi/tractography/SIFT2/tckfactor.h"

#include "dwi/tractography/SIFT/track_contribution.h"




namespace MR {
namespace DWI {
namespace Tractography {
namespace SIFT2 {



FixelExcluder::FixelExcluder (TckFactor& tckfactor, BitSet& to_exclude) :
    master (tckfactor),
    to_exclude (to_exclude),
    mu (master.mu()) { }


FixelExcluder::~FixelExcluder()
{
}



bool FixelExcluder::operator() (const SIFT::TrackIndexRange& range)
{
  for (SIFT::track_t track_index = range.first; track_index != range.second; ++track_index) {
    if (master.coefficients[track_index] == master.max_coeff) {
      const SIFT::TrackContribution& this_contribution (*(master.contributions[track_index]));

      // Task 1: Identify the fixel that should be excluded
      size_t index_to_exclude = 0.0;
      float cost_to_exclude = 0.0;

      // Task 2: Calculate a new coefficient for this streamline
      double weighted_sum = 0.0, sum_weights = 0.0;

      for (size_t j = 0; j != this_contribution.dim(); ++j) {
        const size_t fixel_index = this_contribution[j].get_fixel_index();
        const float length = this_contribution[j].get_value();
        const Fixel& fixel = master.fixels[fixel_index];
        if (!fixel.is_excluded()) {

          // Task 1
          if (fixel.get_diff (mu) < 0.0) {
            const double this_cost = fixel.get_cost (mu) * length / fixel.get_orig_TD();
            if (this_cost > cost_to_exclude) {
              cost_to_exclude = this_cost;
              index_to_exclude = fixel_index;
            }
          }

          // Task 2
          weighted_sum += length * fixel.get_weight() * fixel.get_mean_coeff();
          sum_weights  += length * fixel.get_weight();

        }
      }

      // Task 1
      if (index_to_exclude)
        to_exclude[index_to_exclude] = true;

      // Task 2
      master.coefficients[track_index] = sum_weights ? (weighted_sum / sum_weights) : 0.0;

    }
  }
  return true;
}




}
}
}
}



