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

#include "math/golden_section_search.h"

#include "dwi/tractography/SIFT2/projection.h"
#include "dwi/tractography/SIFT2/tckfactor.h"

#include "dwi/tractography/SIFT/track_contribution.h"



namespace MR {
namespace DWI {
namespace Tractography {
namespace SIFT2 {


/*

ProjectionCalculatorBase::ProjectionCalculatorBase (TckFactor& tckfactor, std::vector<float>& output, StreamlineStats& stats) :
    master (tckfactor),
    mu (master.mu()),
    projected_steps (output),
    output_stats (stats),
    fixel_corr_term_TD_pos (master.fixels.size(), 0.0),
    fixel_corr_term_TD_neg (master.fixels.size(), 0.0),
    local_stats () { }



ProjectionCalculatorBase::ProjectionCalculatorBase (const ProjectionCalculatorBase& that) :
    master (that.master),
    mu (that.mu),
    projected_steps (that.projected_steps),
    output_stats (that.output_stats),
    fixel_corr_term_TD_pos (master.fixels.size(), 0.0),
    fixel_corr_term_TD_neg (master.fixels.size(), 0.0),
    local_stats () { }



ProjectionCalculatorBase::~ProjectionCalculatorBase()
{
  Thread::Mutex::Lock lock (master.mutex);
  for (size_t i = 0; i != master.fixels.size(); ++i) {
    master.fixels[i].add_to_corr_terms (fixel_corr_term_TD_pos[i]);
    master.fixels[i].add_to_corr_terms (fixel_corr_term_TD_neg[i]);
  }
  output_stats += local_stats;
}





bool ProjectionCalculatorBase::operator() (const SIFT::TrackIndexRange& range)
{
  for (SIFT::track_t track_index = range.first; track_index != range.second; ++track_index) {
    if (master.contributions[track_index] && master.contributions[track_index]->dim()) {

      //float projected_step = get_projected_step (track_index);
      float projected_step = 1.0;

      // Don't contribute to the correlation term if the streamline is already at the
      //   maximum permitted weight, and is trying to further increase
      if (master.coefficients[track_index] == master.max_coeff && projected_step > 0.0)
        projected_step = 0.0;

      const float current_weight = Math::exp (master.coefficients[track_index]);

      // TODO Need to extensively test projection fudge with different projection estimators,
      //   different regularisations etc..
      // Probably bronze standard is testing w. GSS for both projection and optimisation
      // In this case, including the fudge is definitely beneficial (both CF data and reg decrease)

      // Fudge here may have to do with the exponential basis...
      // Increasing will tend to overshoot target TD, decreasing will tend to not decrease enough
      // This is however the projected step, NOT the weight!
      // TODO Test a fudge on the projected step, with / without the weight fudge
      // TODO With light regularisation, coefficient histograms are very different...
      // Without it's single peak and fairly symmetrical, with it's weird-shaped (3 bell curves)
      // Need to test with heavier regularisation; even if both the CF and the solution norm are less,
      //   it may not be 'optimal'...
      // TODO Would Shannon entropy be better than solution norm for assessing this?
      projected_step /= current_weight;

      projected_steps[track_index] = projected_step;
      local_stats += projected_step;

      // Contribute to the correlation terms

      // Instead of the instantaneous partial derivative here, try instead using the
      //   change to TD if the streamline were allowed to change according to the projection
      //
      // Instead of:
      // dTd/dFs * dFs_proj = |sl|.e^(Fs).dFs_proj
      //
      // Do:
      // |sl|.e^(Fs+dFs_proj) - |sl|.e^(Fs)
      //
      // The coefficient optimisation then needs to subtract this term from the candidate
      //   from all traversed fixels, and solve for dFs
      // Perhaps the solution domain should be [0,1] as a multiple of the projection value?
      //
      // This is now enabled using #define SIFT2_USE_NEW_PROJECTION
      // In testing, this change proved detrimental
      //

      const SIFT::TrackContribution& this_contribution (*(master.contributions[track_index]));
#ifdef SIFT2_USE_NEW_PROJECTION
      if (projected_step > 0.0) {
        for (size_t j = 0; j != this_contribution.dim(); ++j)
          fixel_corr_term_TD_pos[this_contribution[j].get_fixel_index()] += this_contribution[j].get_value() * (Math::exp (master.coefficients[track_index] + projected_step) - current_weight);
      } else if (projected_step < 0.0) {
        for (size_t j = 0; j != this_contribution.dim(); ++j)
          fixel_corr_term_TD_neg[this_contribution[j].get_fixel_index()] += this_contribution[j].get_value() * (Math::exp (master.coefficients[track_index] + projected_step) - current_weight);
      }
#else
      if (projected_step > 0.0) {
        for (size_t j = 0; j != this_contribution.dim(); ++j)
          fixel_corr_term_TD_pos[this_contribution[j].get_fixel_index()] += this_contribution[j].get_value() * current_weight * projected_step;
      } else if (projected_step < 0.0) {
        for (size_t j = 0; j != this_contribution.dim(); ++j)
          fixel_corr_term_TD_neg[this_contribution[j].get_fixel_index()] += this_contribution[j].get_value() * current_weight * projected_step;
      }
#endif


#ifdef STREAMLINE_OF_INTEREST
      if (track_index == STREAMLINE_OF_INTEREST) {
        std::ofstream out ("soi_ls_projection.csv", std::ios_base::out | std::ios_base::app | std::ios_base::ate);
        LineSearchFunctor line_search_functor (track_index, master);
        for (size_t i = 0; i != 2001; ++i) {
          const float proj = 0.01 * (float(i) - 1000.0);
          if (i)
            out << ",";
          out << line_search_functor (proj);
        }
        out << "\n";
      }
#endif

    }
  }
  return true;
}













ProjectionCalculatorGSS::ProjectionCalculatorGSS (TckFactor& tckfactor, std::vector<float>& output, StreamlineStats& stats) :
    ProjectionCalculatorBase (tckfactor, output, stats) { }

ProjectionCalculatorGSS::ProjectionCalculatorGSS (const ProjectionCalculatorGSS& that) :
    ProjectionCalculatorBase (that) { }



float ProjectionCalculatorGSS::get_projected_step (const SIFT::track_t track_index) const
{
  LineSearchFunctor line_search_functor (track_index, master);
  return Math::golden_section_search (line_search_functor, std::string(""), float(-10.0), float(0.0), float(10.0), float(0.00005));
}







ProjectionCalculatorCombined::ProjectionCalculatorCombined (TckFactor& tckfactor, std::vector<float>& output, StreamlineStats& stats) :
    ProjectionCalculatorBase (tckfactor, output, stats) { }

ProjectionCalculatorCombined::ProjectionCalculatorCombined (const ProjectionCalculatorCombined& that) :
    ProjectionCalculatorBase (that) { }



float ProjectionCalculatorCombined::get_projected_step (const SIFT::track_t track_index) const
{

  static const size_t projection_range = Math::ceil (0.5 * Math::log (float(master.num_tracks()))) + 1;

  //typedef LineSearchFunctor::Result Result;

  std::vector< std::pair<float, float> > search_path;

  //std::cerr << "\n\nNew streamline\n";

  LineSearchFunctor line_search_functor (track_index, master);
  float projected_step = 0.0;
  //float projection_change = 1.0;

  //Result result;

  // Seems it is possible after all to have multiple troughs in the line search
  // Potential thing to try: Calculate the minimum coefficient required to put ALL lobes in excess,
  //   then iterate from there
  / ***
      // First stage: Start at 0.0, then 1.0, continue doubling until the gradient is positive
      do {
        result = line_search_functor (projected_step);
        if (result.first > 0.0) {
          projection_change = 0.0;
        } else {
          search_path.push_back (std::make_pair (projected_step, result.cost));
          projected_step += projection_change;
          projection_change *= 2.0;
        }
      } while (projection_change);
   *** /

  // Looks like when searching from the min_excess point, it's possible to get
  //   stuck on a trough at ~ 5.0 when in fact there's a minimum near 0
  // Possible solution: do two searches

  // Note to self: shape of CF line is why solving with zero regularisation doesn't work well
  //   Can continue to monotonically decrease at very small coefficients - streamline effectively removed


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

  // This is ill-posed, and no longer used; also would often miss minimum below 0.0
  //const float upper_seed = std::min (line_search_functor.min_step_for_all_excess(), float(10.0));
  //const float lower_seed = 0.0;


  if (lower_seed == upper_seed) {

    const Minimum minimum = optimise (line_search_functor, upper_seed, search_path);
    projected_step = minimum.step;

  } else {

    const Minimum first_minimum = optimise (line_search_functor, upper_seed, search_path);
    search_path.push_back (std::make_pair (NAN, NAN));
    const Minimum second_minimum = optimise (line_search_functor, lower_seed, search_path);

    if (first_minimum != failed_search && ((second_minimum == failed_search) || (first_minimum.cf < second_minimum.cf))) {
      projected_step = first_minimum.step;
    } else if (second_minimum != failed_search) {
      projected_step = second_minimum.step;
    } else {
/ ***
    std::cerr << "Index " << track_index << " requires GSS\n";
    projected_step = Math::golden_section_search (line_search_functor,
        std::string(""), float(-10.0), float(0.0), float(10.0), float(0.00005));
    const float gss_cost = line_search_functor (projected_step);

    std::ofstream out_path ((str(track_index) + "_path.csv").c_str(), std::ios_base::trunc);
    for (std::vector< std::pair<float, float> >::const_iterator i = search_path.begin(); i != search_path.end(); ++i)
      out_path << i->first << "," << i->second << "\n";
    out_path << "\n" << projected_step << "," << gss_cost << "\n";

    std::ofstream out_cf ((str(track_index) + "_cf.csv").c_str(), std::ios_base::trunc);
    for (size_t i = 0; i != 2001; ++i) {
      const float proj = 0.01 * (float(i) - 1000.0);
      out_cf << proj << "," << line_search_functor (proj) << "\n";
    }
*** /
  }



  / ***
      // First loop: Pick an appropriate starting point for the line search
      projected_step = line_search_functor.min_coeff_for_all_excess();
      //std::cerr << "Start point for search = " << projected_step << "\n";

      // Perform a Newton optimisation, using the current position of
      //   projected_step as the seed point
      size_t iter = 0;
      do {
        search_path.push_back (std::make_pair<float, float> (projected_step, result.cost));
        result = line_search_functor.get (projected_step);
        // Newton update
        //projection_change = result.second_deriv ? (-result.first_deriv / result.second_deriv) : 0.0;
        // Halley update
        const double numerator   = 2.0 * result.first_deriv * result.second_deriv;
        const double denominator = (2.0 * Math::pow2 (result.second_deriv)) - (result.first_deriv * result.third_deriv);
        projection_change = denominator ? (-numerator / denominator) : 0.0;
        //std::cerr << "Step = " << projected_step << ", result: " << result.cost << " " << result.first_deriv << " " << result.second_deriv << " " << result.third_deriv << ", change " << projection_change << "\n";
        projected_step += projection_change;
      } while ((Math::abs (projection_change) > 0.001) && (++iter < 100));

      search_path.push_back (std::make_pair<float, float> (NAN, NAN));

      result = line_search_functor.get (projected_step);
      const float prev_step = projected_step;
      const float prev_cost = result.cost;
      const size_t prev_iter = iter;

      // Second loop: Start from zero
      // May need some kind of oscillation detection here
      //   (don't want to iterate for ages if only going to use the other result anyway)
      // Probably incorporate this into both attempts - functionalise
      projected_step = 0.0;
      iter = 0;
      do {
        search_path.push_back (std::make_pair<float, float> (projected_step, result.cost));
        result = line_search_functor.get (projected_step);
        //projection_change = result.second ? (-result.first_deriv / result.second_deriv) : 0.0;
        const double numerator   = 2.0 * result.first_deriv * result.second_deriv;
        const double denominator = (2.0 * Math::pow2 (result.second_deriv)) - (result.first_deriv * result.third_deriv);
        projection_change = denominator ? (-numerator / denominator) : 0.0;
        projected_step += projection_change;
      } while ((Math::abs (projection_change) > 0.001) && (++iter < 100));

      result = line_search_functor.get (projected_step);
      float best_cost = result.cost;
      if (best_cost > prev_cost) {
        projected_step = prev_step;
        iter = prev_iter;
        best_cost = prev_cost;
      }

      // Compare to the 'gold standard' - GSS
      const float gss_projected_step = Math::golden_section_search (line_search_functor, std::string(""), float(-100.0), float(0.0), float(100.0), float(0.000005));

      //std::cerr << "Final step = " << projected_step << " with cost " << result.cost << ", GSS has " << gss_projected_step << " at ";
      const float gss_cost = line_search_functor (gss_projected_step);
      //std::cerr << result.cost << "\n";

      // Output only if GSS actually wins
      if (((Math::abs (projected_step - gss_projected_step) > 0.5) || (iter == 100)) && (gss_cost < best_cost)) {

        std::ofstream out_path ((str(track_index) + "_path.csv").c_str(), std::ios_base::trunc);
        for (std::vector< std::pair<float, float> >::const_iterator i = search_path.begin(); i != search_path.end(); ++i)
          out_path << i->first << "," << i->second << "\n";
        out_path << "\n" << gss_projected_step << "," << gss_cost << "\n";

        std::ofstream out_cf ((str(track_index) + "_cf.csv").c_str(), std::ios_base::trunc);
        for (size_t i = 0; i != 2001; ++i) {
          const float proj = 0.01 * (float(i) - 1000.0);
          out_cf << proj << "," << line_search_functor (proj) << "\n";
        }

      }
   *** /

  }

  return projected_step;

}




const ProjectionCalculatorCombined::Minimum ProjectionCalculatorCombined::failed_search = Minimum (0.0, std::numeric_limits<float>::infinity());




ProjectionCalculatorCombined::Minimum
ProjectionCalculatorCombined::optimise (const LineSearchFunctor& functor, const float seed, std::vector< std::pair<float, float> >& search_path) const
{
  typedef LineSearchFunctor::Result Result;
  float step = seed;
  float change = 0.0;
  size_t iter = 0;
  float upper_limit = seed, lower_limit = seed;
  bool enforce_lower_limit = false, enforce_upper_limit = false;
  Result result;
  do {
    search_path.push_back (std::make_pair (step, result.cost));
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
  search_path.push_back (std::make_pair (step, cf));
  return Minimum (step, cf);
}

*/







/*

ProjectionCalculatorQuadratic::ProjectionCalculatorQuadratic (TckFactor& tckfactor, std::vector<float>& output, StreamlineStats& stats) :
    ProjectionCalculatorBase (tckfactor, output, stats) { }

ProjectionCalculatorQuadratic::ProjectionCalculatorQuadratic (const ProjectionCalculatorQuadratic& that) :
    ProjectionCalculatorBase (that) { }



float ProjectionCalculatorQuadratic::get_projected_step (const SIFT::track_t track_index) const
{
  Worker worker (track_index, master);
  return worker.solve();
}






ProjectionCalculatorQuadratic::Worker::Worker (const SIFT::track_t track_index, const TckFactor& master) :
    LineSearchFunctor (track_index, master) { }


float ProjectionCalculatorQuadratic::Worker::solve() const
{

  // FIXME
  throw Exception ("ProjectionCalculatorQuadratic has not been updated with new TV SL regularisation");

  // Construct the terms necessary for solving the quadratic equation
  double a = 0.0, b = 0.0, c = 0.0;
  for (std::vector<Fixel>::const_iterator i = fixels.begin(); i != fixels.end(); ++i) {

    // Fixel exclusion has already taken place in LineSearchFunctor construction

    const float weight = 2.0 * i->PM * i->TD_frac;
    const float roc_TD = i->length * Math::exp (Fs);
    const float diff = (mu * i->TD) - i->FOD;

    a += weight * Math::pow2 (mu * roc_TD);
    b += weight * ((mu * roc_TD * diff) + reg_tv_fixel);
    c += weight * reg_tv_fixel * (Fs - i->meanFs);

  }
  c += 2.0 * reg_tik * Fs;

  float solution_one = (-b - Math::sqrt (b*b - (4.0*a*c))) / (2.0*a);
  float solution_two = (-b + Math::sqrt (b*b - (4.0*a*c))) / (2.0*a);

  // One of these solutions is always zero in the first iteration, later on this is no longer the case
  // But it should be possible to select the appropriate solution by calling the CF functor

  // Transform these according to the exponential nature of the streamline contributions
  solution_one = Math::log (Math::abs (solution_one)) * ((solution_one > 0.0) ? 1.0 : -1.0);
  solution_two = Math::log (Math::abs (solution_two)) * ((solution_two > 0.0) ? 1.0 : -1.0);

  if (!std::isfinite (solution_one))
    return solution_two;
  else if (!std::isfinite (solution_two))
    return solution_one;

  // Both results are 'valid'; find which one is actually the minimum by testing the cost function
  if ((*this) (solution_one) < (*this) (solution_two))
    return solution_one;
  else
    return solution_two;

}


*/





/*

ProjectionCalculatorNewton::ProjectionCalculatorNewton (TckFactor& tckfactor, std::vector<float>& output, StreamlineStats& stats) :
    ProjectionCalculatorBase (tckfactor, output, stats) { }

ProjectionCalculatorNewton::ProjectionCalculatorNewton (const ProjectionCalculatorNewton& that) :
    ProjectionCalculatorBase (that) { }


// Having a straight regularisation penalty for each streamline factor may not prevent
//   streamline coefficients from diverging, even with strong regularisation
// An alternate interpretation of regularisation is that the streamlines attributed to a particular fixel
//   should not deviate significantly in weight from one another
// Rather than storing a 'correlation term' for regularisation for each fixel, instead get a
//   mean and stdev coefficient for each fixel. The regularisation cost is:
//
// f_reg_l = (1 / TD_l_orig) . sum_S{ S_l . (Fs - mean_Fs)^2 }, where mean_Fs = (1 / TD_l_orig) . sum_S{ S_l . Fs }
//
// Then, during the line search, the streamline contributes to the regularisation cost in each fixel it traverses
//   (leave out inter-streamline correlations for now - can try later)
// Also, assume an adequate number of streamlines such that the mean doesn't have to be adjusted within the iteration
//
// f = sum_L{ PM_l . [ (mu.TD_l - FOD_l)^2 + (A.lambda / TD_l_orig).sum_S{ S_l . (Fs - meanFs)^2 } ] }
//
// f_s = sum_L{ PM_l . (S_l / TD_l_orig) . [ (mu.(TD_l + e^(F_S + dF_S).S_l + (dTD_l/dF_S).dF_S) - FOD_l)^2 + lambda.(Fs + dFs - meanFs)^2 ] }
//
//
//
// Gradient calculation:
//
// f_S = sum_L{ PM_l . (S_l / TD_l_orig) . [ (mu.(TD_l + e^(F_S).S_l) - FOD_l)^2  + lambda.A.(F_S - meanFs_l)^2 ] }
//
// df / dF_S = sum_L{ PM_l . (S_l / TD_l_orig) . [ 2 . mu . e^(F_S) . S_l . (mu.(TD_l + e^(F_S).S_l)) - FOD_l) + 2.lambda.A.(F_S - meanFs_l) ] }
//
// d2f / dF_S2 = sum_L{ PM_l . (S_l / TD_l_orig) . [ 2 . mu^2. (e^(F_S))^2 . (S_l)^2 + 2.lambda.A ] }
//


float ProjectionCalculatorNewton::get_projected_step (const SIFT::track_t track_index) const
{
  const SIFT::TrackContribution& this_contribution (*(master.contributions[track_index]));
  const float current_coefficient = master.coefficients[track_index];
  const float current_weight = Math::exp (current_coefficient);

  double gradient = 0.0;
  double second_derivative = 0.0;

  // Alternative scaling of data term
  //double gradient_data = 0.0, gradient_reg = 0.0;
  //double second_deriv_data = 0.0, second_deriv_reg = 0.0;

  for (size_t j = 0; j != this_contribution.dim(); ++j) {
    const Fixel& fixel = master.fixels[this_contribution[j].get_fixel_index()];
    if (!fixel.is_excluded()) {

      const float length = this_contribution[j].get_value();
      const float TD_frac = length / fixel.get_orig_TD();
      const float current_contribution = length * current_weight;
      const float diff = fixel.get_diff (mu);

      // Gradient & second derivative contributions, including TV regularisation
      const float weight = fixel.get_weight() * TD_frac;
      gradient          += 2.0 * weight * ((mu * current_contribution * diff)                                   + (master.reg_multiplier_tv * (current_coefficient - fixel.get_mean_coeff())));
      second_derivative += 2.0 * weight * (((mu * current_contribution) * ((mu * current_contribution) + diff)) + master.reg_multiplier_tv);

      // Alternative scaling of data term
      //gradient_data += 2.0 * fixel.get_weight() * mu * length * current_weight * fixel.get_diff (mu);
      //gradient_reg  += 2.0 * fixel.get_weight() * master.reg_multiplier_tv * TD_frac * (current_coefficient - fixel.get_mean_coeff());
      //second_deriv_data += 2.0 * fixel.get_weight() * Math::pow2 (mu * length * current_weight);
      //second_deriv_reg  += 2.0 * fixel.get_weight() * master.reg_multiplier_tv * TD_frac;

    }
  }

  // Add components to gradient & second derivative from Tikhonov regularisation
  gradient          += 2.0 * master.reg_multiplier_tikhonov * current_coefficient;
  second_derivative += 2.0 * master.reg_multiplier_tikhonov;

  // Alternative scaling of data term
  //gradient_data *= this_contribution.get_total_contribution() / master.TD_sum;
  //second_deriv_data *= this_contribution.get_total_contribution() / master.TD_sum;
  //gradient_reg += 2.0 * master.reg_multiplier_tikhonov * current_coefficient;
  //second_deriv_reg += 2.0 * master.reg_multiplier_tikhonov;
  //const double gradient = gradient_data + gradient_reg;
  //const double second_derivative = second_deriv_data + second_deriv_reg;

  // Projected step based on Newton update
  float projected_step = second_derivative ? (- gradient / second_derivative) : 0.0;

  // Should be able to detect whether this is in fact converging toward a minimum or a maximum
  // If heading toward a maximum, could negate or set to zero
  // For testing purposes, log the number of streamlines for which this is the case
  if (second_derivative < 0.0)
    projected_step = -projected_step;

  return projected_step;
}









ProjectionCalculatorNRIterative::ProjectionCalculatorNRIterative (TckFactor& tckfactor, std::vector<float>& output, StreamlineStats& stats) :
    ProjectionCalculatorBase (tckfactor, output, stats) { }

ProjectionCalculatorNRIterative::ProjectionCalculatorNRIterative (const ProjectionCalculatorNRIterative& that) :
    ProjectionCalculatorBase (that) { }



float ProjectionCalculatorNRIterative::get_projected_step (const SIFT::track_t track_index) const
{

  std::vector< std::pair<float, float> > search_path;

  const SIFT::TrackContribution& this_contribution (*(master.contributions[track_index]));
  float projected_step = 0.0;
  float projection_change = 0.0;
  size_t iter = 0;

  do {

    const float current_coefficient = master.coefficients[track_index] + projected_step;
    const float current_weight = Math::exp (current_coefficient);

    double gradient = 0.0, second_derivative = 0.0;
    double cost = 0.0;
    for (size_t j = 0; j != this_contribution.dim(); ++j) {
      const Fixel& fixel = master.fixels[this_contribution[j].get_fixel_index()];
      if (!fixel.is_excluded()) {
        const float length = this_contribution[j].get_value();
        const float current_contribution = length * current_weight;
        const double new_TD = fixel.get_TD() + (length * (Math::exp(current_coefficient) - Math::exp(master.coefficients[track_index])));
        const double new_diff = (mu * new_TD) - fixel.get_FOD();
        const float TD_frac = length / fixel.get_orig_TD();
        const float weight = fixel.get_weight() * TD_frac;
        gradient          += 2.0 * weight * ((mu * current_contribution * new_diff) + (master.reg_multiplier_tv * (current_coefficient - fixel.get_mean_coeff())));
        second_derivative += 2.0 * weight * (((mu * current_contribution) * ((mu * current_contribution) + new_diff)) + master.reg_multiplier_tv);
        cost += weight * (Math::pow2 (new_diff) + (master.reg_multiplier_tv * Math::pow2 (current_coefficient - fixel.get_mean_coeff())));
      }
    }
    gradient          += 2.0 * master.reg_multiplier_tikhonov * current_coefficient;
    second_derivative += 2.0 * master.reg_multiplier_tikhonov;
    cost += (master.reg_multiplier_tikhonov * Math::pow2 (current_coefficient));

    search_path.push_back (std::make_pair<float, float> (projected_step, cost));

    projection_change = second_derivative ? (- gradient / second_derivative) : 0.0;

    // Limit the distance covered by a single iteration
    // Made the two limits slightly different to reduce risk of oscillation in rare circumstances
    if (projection_change > 5.25)
      projection_change = 5.25;
    else if (projection_change < -4.75)
      projection_change = -4.75;

    // If heading toward a maximum, go in the other direction
    // Hopefully a small step will be more reliable than inverting (less likely to oscillate or step wildly)
    // Small difference between the two limits to prevent oscillation in very rare circumstances
    if (second_derivative < 0.0)
      projection_change = (gradient < 0.0 ? +0.525 : -0.475);

    projected_step += projection_change;

    // FIXME Would prefer to know that the optimiser has in fact looked beyond the minimum;
    //   otherwise, it may be stuck on a plateau
    // Wait until comparison with GSS has been performed, see if it's a problem
    // TODO Yes, looks like some are getting stuck on the plateau
    // Some are close but ~ 0.05 off; reduced threshold for re-iterating, but may be hitting float accuracy limit
    // Others are way off
  } while ((Math::abs (projection_change) > 0.001) && (++iter < 100));

  LineSearchFunctor line_search_functor (track_index, master);
  const float gss_projection = Math::golden_section_search (line_search_functor, std::string(""), float(-100.0), float(0.0), float(100.0), float(0.000005));
  if (Math::abs (projected_step - gss_projection) > 0.1) {
    const float proj_cost = line_search_functor (projected_step);
    const float gss_cost  = line_search_functor (gss_projection);
    std::cerr << "Streamline " << track_index << ": Projection " << projected_step << " = " << proj_cost << "; GSS " << gss_projection << " = " << gss_cost << "\n";

    //if (iter == 100) {

    // Generate 2 plots: first is cf as a function of projected_step
    //   second is path of line search (projected_step / cf pairs)
    {
      std::ofstream out_path ((str(track_index) + "_path.csv").c_str(), std::ios_base::trunc);
      for (std::vector< std::pair<float, float> >::const_iterator i = search_path.begin(); i != search_path.end(); ++i)
        out_path << i->first << "," << i->second << "\n";
    }

    std::ofstream out_cf ((str(track_index) + "_cf.csv").c_str(), std::ios_base::trunc);
    out_cf << "Projection,Cost,Gradient,Second_derivative\n";
    for (size_t i = 0; i != 2001; ++i) {
      const float proj = 0.01 * (float(i) - 1000.0);
      const float current_coefficient = master.coefficients[track_index] + proj;
      const float current_weight = Math::exp (current_coefficient);
      double cost = 0.0, gradient = 0.0, second_derivative = 0.0;
      for (size_t j = 0; j != this_contribution.dim(); ++j) {
        const Fixel& fixel = master.fixels[this_contribution[j].get_fixel_index()];
        if (!fixel.is_excluded()) {
          const float length = this_contribution[j].get_value();
          const float current_contribution = length * current_weight;
          const double new_TD = fixel.get_TD() + (length * (Math::exp(current_coefficient) - Math::exp(master.coefficients[track_index])));
          const double new_diff = (mu * new_TD) - fixel.get_FOD();
          const float TD_frac = length / fixel.get_orig_TD();
          const float weight = fixel.get_weight() * TD_frac;
          gradient          += 2.0 * weight * ((mu * current_contribution * new_diff) + (master.reg_multiplier_tv * (current_coefficient - fixel.get_mean_coeff())));
          second_derivative += 2.0 * weight * (((mu * current_contribution) * ((mu * current_contribution) + new_diff)) + master.reg_multiplier_tv);
          cost += weight * (Math::pow2 (new_diff) + (master.reg_multiplier_tv * Math::pow2 (current_coefficient - fixel.get_mean_coeff())));
        }
      }
      cost += (master.reg_multiplier_tikhonov * Math::pow2 (current_coefficient));
      out_cf << proj << "," << cost << "," << gradient << "," << second_derivative << "\n";
    }

    LineSearchFunctor line_search_functor (track_index, master);
    projected_step = Math::golden_section_search (line_search_functor, std::string(""), float(-100.0), float(0.0), float(100.0), float(0.000005));

  }

  return projected_step;
}









ProjectionCalculatorHalleyIterative::ProjectionCalculatorHalleyIterative (TckFactor& tckfactor, std::vector<float>& output, StreamlineStats& stats) :
    ProjectionCalculatorBase (tckfactor, output, stats) { }

ProjectionCalculatorHalleyIterative::ProjectionCalculatorHalleyIterative (const ProjectionCalculatorHalleyIterative& that) :
    ProjectionCalculatorBase (that) { }



float ProjectionCalculatorHalleyIterative::get_projected_step (const SIFT::track_t track_index) const
{

  std::vector< std::pair<float, float> > search_path;

  const SIFT::TrackContribution& this_contribution (*(master.contributions[track_index]));
  float projected_step = 0.0;
  float projection_change = 0.0;
  size_t iter = 0;

  do {

    const float current_coefficient = master.coefficients[track_index] + projected_step;
    const float current_weight = Math::exp (current_coefficient);

    double gradient = 0.0, second_derivative = 0.0, third_derivative = 0.0;
    double cost = 0.0;
    for (size_t j = 0; j != this_contribution.dim(); ++j) {
      const Fixel& fixel = master.fixels[this_contribution[j].get_fixel_index()];
      if (!fixel.is_excluded()) {
        const float length = this_contribution[j].get_value();
        const float current_contribution = length * current_weight;
        const double new_TD = fixel.get_TD() + (length * (Math::exp(current_coefficient) - Math::exp(master.coefficients[track_index])));
        const double new_diff = (mu * new_TD) - fixel.get_FOD();
        const float TD_frac = length / fixel.get_orig_TD();
        const float weight = fixel.get_weight() * TD_frac;
        gradient          += 2.0 * weight * ((mu * current_contribution * new_diff) + (master.reg_multiplier_tv * (current_coefficient - fixel.get_mean_coeff())));
        second_derivative += 2.0 * weight * (((mu * current_contribution) * ((mu * current_contribution) + new_diff)) + master.reg_multiplier_tv);
        third_derivative  += 2.0 * weight * (mu * current_contribution) * ((3.0 * mu * current_contribution) + new_diff);
        cost += weight * (Math::pow2 (new_diff) + (master.reg_multiplier_tv * Math::pow2 (current_coefficient - fixel.get_mean_coeff())));
      }
    }
    gradient          += 2.0 * master.reg_multiplier_tikhonov * current_coefficient;
    second_derivative += 2.0 * master.reg_multiplier_tikhonov;
    cost += (master.reg_multiplier_tikhonov * Math::pow2 (current_coefficient));

    search_path.push_back (std::make_pair<float, float> (projected_step, cost));

    // Halley update
    const double numerator   = 2.0 * gradient * second_derivative;
    const double denominator = (2.0 * Math::pow2 (second_derivative)) - (gradient * third_derivative);
    projection_change = denominator ? (-numerator / denominator) : 0.0;

    // Still possible to oscillate, if the subsequent update undoes the guess
    // Better would be to track the upper and lower bounds, and do a bisection if it gets
    //   close to one of these bounds or tries to go beyond it
    if (projection_change > 5.25)
      projection_change = 5.25;
    else if (projection_change < -4.75)
      projection_change = -4.75;

    if (second_derivative < 0.0)
      projection_change = (gradient < 0.0 ? +0.525 : -0.475);

    projected_step += projection_change;

  } while ((Math::abs (projection_change) > 0.001) && (++iter < 100));

  // Had expected Halley's method to fail less frequently at later iterations;
  //   in fact, it fails more often...

  if (iter == 100) {

    VAR (track_index);

    {
      std::ofstream out_path ((str(track_index) + "_path.csv").c_str(), std::ios_base::trunc);
      for (std::vector< std::pair<float, float> >::const_iterator i = search_path.begin(); i != search_path.end(); ++i)
        out_path << i->first << "," << i->second << "\n";
    }

    std::ofstream out_cf ((str(track_index) + "_cf.csv").c_str(), std::ios_base::trunc);
    for (size_t i = 0; i != 2001; ++i) {
      const float proj = 0.01 * (float(i) - 1000.0);
      const float current_coefficient = master.coefficients[track_index] + proj;
      double cost = 0.0;
      for (size_t j = 0; j != this_contribution.dim(); ++j) {
        const Fixel& fixel = master.fixels[this_contribution[j].get_fixel_index()];
        if (!fixel.is_excluded()) {
          const float length = this_contribution[j].get_value();
          const float TD_frac = length / fixel.get_orig_TD();
          const float weight = fixel.get_weight() * TD_frac;
          const double new_TD = fixel.get_TD() + (length * (Math::exp(current_coefficient) - Math::exp(master.coefficients[track_index])));
          const double new_diff = (mu * new_TD) - fixel.get_FOD();
          cost += weight * (Math::pow2 (new_diff) + (master.reg_multiplier_tv * Math::pow2 (current_coefficient - fixel.get_mean_coeff())));
        }
      }
      cost += (master.reg_multiplier_tikhonov * Math::pow2 (current_coefficient));
      out_cf << proj << "," << cost << "\n";
    }

    LineSearchFunctor line_search_functor (track_index, master);
    projected_step = Math::golden_section_search (line_search_functor, std::string(""), float(-100.0), float(0.0), float(100.0), float(0.000005));

  }

  return projected_step;
}

*/




}
}
}
}



