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

#include "dwi/tractography/SIFT2/iterative_projection.h"
#include "dwi/tractography/SIFT2/tckfactor.h"

#include "dwi/tractography/SIFT/track_contribution.h"



namespace MR {
namespace DWI {
namespace Tractography {
namespace SIFT2 {





IterativeProjection::IterativeProjection (TckFactor& tckfactor, std::vector<float>& output, StreamlineStats& stats, size_t& sign_flip) :
    ProjectionCalculatorBase (tckfactor, output, stats),
    sign_flip (sign_flip),
    local_sign_flip (0)
{
  corr_zeroed = false;
}

IterativeProjection::IterativeProjection (const IterativeProjection& that) :
    ProjectionCalculatorBase (that),
    sign_flip (that.sign_flip),
    local_sign_flip (0) { }

IterativeProjection::~IterativeProjection ()
{
  Thread::Mutex::Lock lock (master.mutex);
  if (!corr_zeroed) {
    corr_zeroed = true;
    for (std::vector<Fixel>::iterator i = master.fixels.begin(); i != master.fixels.end(); ++i)
      i->clear_corr_terms();
  }
  sign_flip += local_sign_flip;
}



float IterativeProjection::get_projected_step (const SIFT::track_t track_index) const
{

  if (corr_zeroed)
    throw Exception ("Correlation terms zeroed before processing completed!");

  static const size_t projection_range = Math::ceil (0.5 * Math::log (float(master.num_tracks()))) + 1;

  // TODO Line search won't do anything if the projected step is zero;
  //   then again, if it goes to zero during iteration, don't want to
  //   give it zero correlations either
  // Think the best solution is going to be to use ProjectionCalculatorCombined
  //   for the first iteration, then loop this class
  // That way a zero projection can be used to flag that no further optimisation can take place

  if (!prev_projected_step (track_index))
    return 0.0;

  LineSearchFunctor line_search_functor (track_index, master, prev_projected_step (track_index));
  float projected_step = 0.0;

  float upper_seed = projection_range;
  float lower_seed = -projection_range;

  float seed_cf = line_search_functor (upper_seed);
  float new_cf;
  do {
    const float new_seed = upper_seed - float(1.0);
    new_cf = line_search_functor (new_seed);
    if (!std::isfinite (seed_cf) || (new_cf <= seed_cf)) {
      upper_seed = new_seed;
      seed_cf = new_cf;
    }
  } while (new_cf <= seed_cf);

  seed_cf = line_search_functor (lower_seed);
  do {
    const float new_seed = lower_seed + float(1.0);
    new_cf = line_search_functor (new_seed);
    if (new_cf <= seed_cf) {
      lower_seed = new_seed;
      seed_cf = new_cf;
    }
  } while (new_cf <= seed_cf);


  if (lower_seed == upper_seed) {

    const Minimum minimum = optimise (line_search_functor, upper_seed);
    projected_step = minimum.step;

  } else {

    const Minimum first_minimum = optimise (line_search_functor, upper_seed);
    const Minimum second_minimum = optimise (line_search_functor, lower_seed);

    if (first_minimum != failed_search && ((second_minimum == failed_search) || (first_minimum.cf < second_minimum.cf))) {
      projected_step = first_minimum.step;
    } else if (second_minimum != failed_search) {
      projected_step = second_minimum.step;
    }

  }

  // Still getting some invalids; get the line plots
  // First example seems to be just a tiny (but non-zero) projection, resulting in floating-point errors
  if (!projected_step || !std::isfinite (projected_step)) {
/*
    {
      std::ofstream out_cf ((str(track_index) + "_cf.csv").c_str(), std::ios_base::trunc);
      for (size_t i = 0; i != 2001; ++i) {
        const float proj = 0.01 * (float(i) - 1000.0);
        out_cf << proj << "," << line_search_functor (proj) << "\n";
      }
    }
    VAR (prev_projected_step (track_index));

    exit (1);
*/
    projected_step = 0.0;
  }

  if (projected_step && ((projected_step < 0.0 && prev_projected_step (track_index) > 0.0) || (projected_step > 0.0 && prev_projected_step (track_index) < 0.0)))
    ++local_sign_flip;

  return projected_step;

}




const IterativeProjection::Minimum IterativeProjection::failed_search = Minimum (0.0, std::numeric_limits<float>::infinity());




IterativeProjection::Minimum
IterativeProjection::optimise (const LineSearchFunctor& functor, const float seed) const
{
  typedef LineSearchFunctor::Result Result;
  float step = seed;
  float change = 0.0;
  size_t iter = 0;
  float upper_limit = seed, lower_limit = seed;
  bool enforce_lower_limit = false, enforce_upper_limit = false;
  Result result;
  do {
    result = functor.get (step);
    if (result.valid()) {

      // Newton update
      //change = result.second_deriv ? (-result.first_deriv / result.second_deriv) : 0.0;
      // Halley update - faster & doesn't appear to incur any errors
      const double numerator   = 2.0 * result.first_deriv * result.second_deriv;
      const double denominator = (2.0 * Math::pow2 (result.second_deriv)) - (result.first_deriv * result.third_deriv);
      change = denominator ? (-numerator / denominator) : 0.0;

      if (result.second_deriv < 0.0) // Local maxima
        change = -change;

    } else {
      change = -1.0;
    }
    step += change;
    if (step > upper_limit) {
      if (enforce_upper_limit)
        return failed_search;
      else
        upper_limit = step;
    } else {
      enforce_upper_limit = true;
    }
    if (step < lower_limit) {
      if (enforce_lower_limit)
        return failed_search;
      else
        lower_limit = step;
    } else {
      enforce_lower_limit = true;
    }
  } while ((Math::abs (change) > 0.001) && (++iter < 100));
  if (iter == 100)
    return failed_search;
  const float cf = functor (step);
  return Minimum (step, cf);
}




bool IterativeProjection::corr_zeroed = false;




}
}
}
}



