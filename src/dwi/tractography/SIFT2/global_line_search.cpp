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


#include "dwi/tractography/SIFT2/fixel.h"
#include "dwi/tractography/SIFT2/fixel_updater.h"
#include "dwi/tractography/SIFT2/global_line_search.h"
#include "dwi/tractography/SIFT2/reg_calculator.h"
#include "dwi/tractography/SIFT2/tckfactor.h"


namespace MR {
namespace DWI {
namespace Tractography {
namespace SIFT2 {



GlobalLineSearchFunctor::GlobalLineSearchFunctor (TckFactor& tckfactor, const std::vector<float>& projections) :
    master (tckfactor),
    orig_coeffs (tckfactor.coefficients),
    projected_steps (projections),
    cf_data (0.0),
    cf_reg_tik (0.0),
    cf_reg_tv_fixel (0.0),
    cf_reg_tv_sl (0.0) { }



float GlobalLineSearchFunctor::operator() (const float step)
{
  //std::cerr << step << " ";
  // Set up the per-streamline coefficients
  for (SIFT::track_t track_index = 0; track_index != master.num_tracks(); ++track_index) {
    master.coefficients[track_index] = orig_coeffs[track_index] + (step * projected_steps[track_index]);
    //if (!std::isfinite (master.coefficients[track_index])) {
    //  VAR (master.coefficients[track_index]);
    //  VAR (track_index);
    //  VAR (projected_steps[track_index]);
    //  VAR (master.contributions[track_index]->dim());
    //}
  }

  // Update the streamline density, and mean weighting coefficient, in each fixel
  for (std::vector<Fixel>::iterator i = master.fixels.begin(); i != master.fixels.end(); ++i) {
    i->clear_TD();
    i->clear_mean_coeff();
  }
  {
    SIFT::TrackIndexRangeWriter writer (SIFT_TRACK_INDEX_BUFFER_SIZE, master.num_tracks());
    FixelUpdater worker (master);
    Thread::run_queue (writer, SIFT::TrackIndexRange(), Thread::multi (worker));
  }
  for (std::vector<Fixel>::iterator i = master.fixels.begin(); i != master.fixels.end(); ++i)
    i->normalise_mean_coeff();

  // Calculate the data component of the cost function
  cf_data = master.calc_cost_function();

  // Calculate the regularisation component of the cost function
  cf_reg_tik = cf_reg_tv_fixel = cf_reg_tv_sl = 0.0;
  {
    SIFT::TrackIndexRangeWriter writer (SIFT_TRACK_INDEX_BUFFER_SIZE, master.num_tracks());
    RegularisationCalculator worker (master, cf_reg_tik, cf_reg_tv_fixel, cf_reg_tv_sl);
    Thread::run_queue (writer, SIFT::TrackIndexRange(), Thread::multi (worker));
  }
  cf_reg_tik      *= master.reg_multiplier_tikhonov;
  cf_reg_tv_fixel *= master.reg_multiplier_tv_fixel;
  cf_reg_tv_sl    *= master.reg_multiplier_tv_streamline;

  return (cf_data + cf_reg_tik + cf_reg_tv_fixel + cf_reg_tv_sl);
}



}
}
}
}

