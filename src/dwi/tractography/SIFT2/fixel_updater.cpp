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

#include "dwi/tractography/SIFT2/fixel_updater.h"
#include "dwi/tractography/SIFT2/tckfactor.h"

#include "dwi/tractography/SIFT/track_contribution.h"




namespace MR {
  namespace DWI {
    namespace Tractography {
      namespace SIFT2 {



      FixelUpdater::FixelUpdater (TckFactor& tckfactor) :
        master (tckfactor),
        fixel_coeff_sums (tckfactor.fixels.size(), 0.0),
        fixel_TDs        (tckfactor.fixels.size(), 0.0),
        fixel_counts     (tckfactor.fixels.size(), 0) { }



      FixelUpdater::~FixelUpdater()
      {
        std::lock_guard<std::mutex> lock (master.mutex);
        for (size_t i = 0; i != master.fixels.size(); ++i) {
          master.fixels[i].add_to_mean_coeff (fixel_coeff_sums[i]);
          master.fixels[i].add_TD (fixel_TDs[i], fixel_counts[i]);
        }
      }



      bool FixelUpdater::operator() (const SIFT::TrackIndexRange& range)
      {
        for (SIFT::track_t track_index = range.first; track_index != range.second; ++track_index) {
          const float coefficient = master.coefficients[track_index];
          const SIFT::TrackContribution& this_contribution (*(master.contributions[track_index]));
          const float weighting_factor = (coefficient > master.min_coeff) ? std::exp (coefficient) : 0.0;
          for (size_t j = 0; j != this_contribution.dim(); ++j) {
            const size_t fixel_index = this_contribution[j].get_fixel_index();
            const float length = this_contribution[j].get_length();
            fixel_coeff_sums[fixel_index] += length * coefficient;
            fixel_TDs       [fixel_index] += length * weighting_factor;
            fixel_counts    [fixel_index]++;
          }
        }
        return true;
      }




      }
    }
  }
}


