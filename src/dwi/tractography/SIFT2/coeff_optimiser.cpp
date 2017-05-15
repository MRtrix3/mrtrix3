/* Copyright (c) 2008-2017 the MRtrix3 contributors.
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


#include <mutex>

#include "dwi/tractography/SIFT2/coeff_optimiser.h"
#include "dwi/tractography/SIFT2/line_search.h"
#include "dwi/tractography/SIFT2/tckfactor.h"




namespace MR {
  namespace DWI {
    namespace Tractography {
      namespace SIFT2 {





      CoefficientOptimiserBase::CoefficientOptimiserBase (TckFactor& tckfactor, StreamlineStats& step_stats, StreamlineStats& coefficient_stats, unsigned int& nonzero_streamlines, BitSet& fixels_to_exclude, double& sum_costs) :
            master (tckfactor),
            mu (tckfactor.mu()),
#ifdef SIFT2_COEFF_OPTIMISER_DEBUG
            total (0),
            failed (0),
            wrong_dir (0),
            step_truncated (0),
            coeff_truncated (0),
#endif
            step_stats (step_stats),
            coefficient_stats (coefficient_stats),
            nonzero_streamlines (nonzero_streamlines),
            fixels_to_exclude (fixels_to_exclude),
            sum_costs (sum_costs),
            local_stats_steps (),
            local_stats_coefficients (),
            local_nonzero_count (0),
            local_to_exclude (fixels_to_exclude.size()),
            local_sum_costs (0.0) { }



      CoefficientOptimiserBase::CoefficientOptimiserBase (const CoefficientOptimiserBase& that) :
            master (that.master),
            mu (that.mu),
#ifdef SIFT2_COEFF_OPTIMISER_DEBUG
            total (0),
            failed (0),
            wrong_dir (0),
            step_truncated (0),
            coeff_truncated (0),
#endif
            step_stats (that.step_stats),
            coefficient_stats (that.coefficient_stats),
            nonzero_streamlines (that.nonzero_streamlines),
            fixels_to_exclude (that.fixels_to_exclude),
            sum_costs (that.sum_costs),
            local_stats_steps (),
            local_stats_coefficients (),
            local_nonzero_count (0),
            local_to_exclude (fixels_to_exclude.size()),
            local_sum_costs (0.0) { }



      CoefficientOptimiserBase::~CoefficientOptimiserBase()
      {
        std::lock_guard<std::mutex> lock (master.mutex);
#ifdef SIFT2_COEFF_OPTIMISER_DEBUG
        fprintf (stderr, "%ld of %ld initial searches failed, %ld in wrong direction, %ld steps truncated, %ld coefficients truncated\n", failed, total, wrong_dir, step_truncated, coeff_truncated);
#endif
        step_stats += local_stats_steps;
        coefficient_stats += local_stats_coefficients;
        nonzero_streamlines += local_nonzero_count;
        fixels_to_exclude |= local_to_exclude;
        sum_costs += local_sum_costs;
      }



      bool CoefficientOptimiserBase::operator() (const SIFT::TrackIndexRange& range)
      {

        for (SIFT::track_t track_index = range.first; track_index != range.second; ++track_index) {

          double dFs = get_coeff_change (track_index);

#ifdef SIFT2_COEFF_OPTIMISER_DEBUG
          ++total;
#endif

          if (dFs >= master.max_coeff_step) {
            dFs = master.max_coeff_step;
#ifdef SIFT2_COEFF_OPTIMISER_DEBUG
            ++step_truncated;
#endif
          } else if (dFs <= -master.max_coeff_step) {
            dFs = -master.max_coeff_step;
#ifdef SIFT2_COEFF_OPTIMISER_DEBUG
            ++step_truncated;
#endif
          }

          const double old_coefficient = master.coefficients[track_index];
          double new_coefficient = old_coefficient + dFs;
          if (new_coefficient < master.min_coeff) {
            new_coefficient = master.min_coeff;
            dFs = master.min_coeff - old_coefficient;
#ifdef SIFT2_COEFF_OPTIMISER_DEBUG
            ++coeff_truncated;
#endif
          } else if (new_coefficient > master.max_coeff) {
            new_coefficient = do_fixel_exclusion (track_index);
            dFs = master.max_coeff - old_coefficient;
#ifdef SIFT2_COEFF_OPTIMISER_DEBUG
            ++coeff_truncated;
#endif
          }

          master.coefficients[track_index] = new_coefficient;

          // Update the stats
          local_stats_steps += dFs;
          local_stats_coefficients += new_coefficient;
          if (master.contributions[track_index] && master.contributions[track_index]->dim() && new_coefficient > master.min_coeff)
            ++local_nonzero_count;

#ifdef STREAMLINE_OF_INTEREST
          if (track_index == STREAMLINE_OF_INTEREST) {

            {
              std::ofstream out ("soi_general.csv", std::ios_base::out | std::ios_base::app | std::ios_base::ate);
              out << old_coefficient << "," << this_projected_step << "," << dFs << "," << new_coefficient << "\n";
            }
            {
              std::ofstream out ("soi_ls_coeff.csv", std::ios_base::out | std::ios_base::app | std::ios_base::ate);
              LineSearchFunctor line_search_functor (track_index, master, this_projected_step);
              for (size_t i = 0; i != 2001; ++i) {
                const float dFs = 0.001 * (float(i) - 1000.0);
                if (i)
                  out << ",";
                out << line_search_functor (dFs);
              }
              out << "\n";
            }

          }
#endif

        }

        return true;

      }



      double CoefficientOptimiserBase::do_fixel_exclusion (const SIFT::track_t track_index)
      {
        const SIFT::TrackContribution& this_contribution (*(master.contributions[track_index]));

        // Task 1: Identify the fixel that should be excluded
        size_t index_to_exclude = 0.0;
        float cost_to_exclude = 0.0;

        for (size_t j = 0; j != this_contribution.dim(); ++j) {
          const size_t fixel_index = this_contribution[j].get_fixel_index();
          const float length = this_contribution[j].get_length();
          const Fixel& fixel = master.fixels[fixel_index];
          if (!fixel.is_excluded() && (fixel.get_diff (mu) < 0.0)) {

            const double this_cost = fixel.get_cost (mu) * length / fixel.get_orig_TD();
            if (this_cost > cost_to_exclude) {
              cost_to_exclude = this_cost;
              index_to_exclude = fixel_index;
            }

          }
        }

        if (index_to_exclude)
          local_to_exclude[index_to_exclude] = true;
        else
          return 0.0;

        // Task 2: Calculate a new coefficient for this streamline
        double weighted_sum = 0.0, sum_weights = 0.0;

        for (size_t j = 0; j != this_contribution.dim(); ++j) {
          const size_t fixel_index = this_contribution[j].get_fixel_index();
          const float length = this_contribution[j].get_length();
          const Fixel& fixel = master.fixels[fixel_index];
          if (!fixel.is_excluded() && (fixel_index != index_to_exclude)) {

            weighted_sum += length * fixel.get_weight() * fixel.get_mean_coeff();
            sum_weights  += length * fixel.get_weight();

          }
        }

        return (sum_weights ? (weighted_sum / sum_weights) : 0.0);
      }












      CoefficientOptimiserGSS::CoefficientOptimiserGSS (TckFactor& tckfactor, StreamlineStats& step_stats, StreamlineStats& coefficient_stats, unsigned int& nonzero_streamlines, BitSet& fixels_to_exclude, double& sum_costs) :
            CoefficientOptimiserBase (tckfactor, step_stats, coefficient_stats, nonzero_streamlines, fixels_to_exclude, sum_costs) { }

      CoefficientOptimiserGSS::CoefficientOptimiserGSS (const CoefficientOptimiserGSS& that) :
            CoefficientOptimiserBase (that) { }

      double CoefficientOptimiserGSS::get_coeff_change (const SIFT::track_t track_index) const
      {
        LineSearchFunctor line_search_functor (track_index, master);
        const double dFs = Math::golden_section_search (line_search_functor, std::string(""), -master.max_coeff_step, 0.0, master.max_coeff_step, 0.001 / (2.0 * master.max_coeff_step));
        const double cost = line_search_functor (dFs);
        // The Golden Section Search doesn't check the endpoints; do that here
        // TODO Once GSS is turned into a functor, add this capability to it
        if ((dFs > 0.99 * master.max_coeff_step) && (line_search_functor (master.max_coeff_step) < cost))
          return master.max_coeff_step;
        if ((dFs < -0.99 * master.max_coeff_step) && (line_search_functor (-master.max_coeff_step) < cost))
          return -master.max_coeff_step;
        return dFs;
      }







      CoefficientOptimiserQLS::CoefficientOptimiserQLS (TckFactor& tckfactor, StreamlineStats& step_stats, StreamlineStats& coefficient_stats, unsigned int& nonzero_streamlines, BitSet& fixels_to_exclude, double& sum_costs) :
            CoefficientOptimiserBase (tckfactor, step_stats, coefficient_stats, nonzero_streamlines, fixels_to_exclude, sum_costs),
            qls (-master.max_coeff_step, master.max_coeff_step)
      {
        qls.set_exit_if_outside_bounds (false);
        qls.set_value_tolerance (0.001);
      }

      CoefficientOptimiserQLS::CoefficientOptimiserQLS (const CoefficientOptimiserQLS& that) :
            CoefficientOptimiserBase (that),
            qls (-master.max_coeff_step, master.max_coeff_step)
      {
        qls.set_exit_if_outside_bounds (false);
        qls.set_value_tolerance (0.001);
      }

      double CoefficientOptimiserQLS::get_coeff_change (const SIFT::track_t track_index) const
      {
        LineSearchFunctor line_search_functor (track_index, master);

        double dFs = qls (line_search_functor);
#ifdef SIFT2_COEFF_OPTIMISER_DEBUG
        ++total;
#endif
        if (!std::isfinite (dFs)) {
          dFs = Math::golden_section_search (line_search_functor, std::string(""), -master.max_coeff_step, 0.0, master.max_coeff_step, 0.001 / (2.0 * master.max_coeff_step));
          double cost = line_search_functor (dFs);
          if (dFs > 0.99 * master.max_coeff_step && line_search_functor (master.max_coeff_step) < cost)
            dFs = master.max_coeff_step;
          else if (dFs < -0.99 * master.max_coeff_step && line_search_functor (-master.max_coeff_step) < cost)
            dFs = -master.max_coeff_step;
#ifdef SIFT2_COEFF_OPTIMISER_DEBUG
          ++failed;
#endif
        }

        return dFs;
      }








      CoefficientOptimiserIterative::CoefficientOptimiserIterative (TckFactor& tckfactor, StreamlineStats& step_stats, StreamlineStats& coefficient_stats, unsigned int& nonzero_streamlines, BitSet& fixels_to_exclude, double& sum_costs) :
            CoefficientOptimiserBase (tckfactor, step_stats, coefficient_stats, nonzero_streamlines, fixels_to_exclude, sum_costs)
#ifdef SIFT2_COEFF_OPTIMISER_DEBUG
      , iter_count (0)
#endif
      { }

      CoefficientOptimiserIterative::CoefficientOptimiserIterative (const CoefficientOptimiserIterative& that) :
            CoefficientOptimiserBase (that)
#ifdef SIFT2_COEFF_OPTIMISER_DEBUG
      , iter_count (0)
#endif
      { }

      CoefficientOptimiserIterative::~CoefficientOptimiserIterative()
      {
#ifdef SIFT2_COEFF_OPTIMISER_DEBUG
        Thread::Mutex::Lock lock (master.mutex);
        fprintf (stderr, "Mean number of iterations: %f\n", iter_count / float(total));
#endif
      }

      double CoefficientOptimiserIterative::get_coeff_change (const SIFT::track_t track_index) const
      {

        LineSearchFunctor line_search_functor (track_index, master);

        double dFs = 0.0;
        double change = 0.0;
        size_t iter = 0;
        do {

          const LineSearchFunctor::Result result = line_search_functor.get (dFs);

          // Newton update
          change = result.second_deriv ? (-result.first_deriv / result.second_deriv) : 0.0;
          if (result.second_deriv < 0.0)
            change = -change;

          // Halley update
          //const double numerator   = 2.0 * result.first_deriv * result.second_deriv;
          //const double denominator = (2.0 * Math::pow2 (result.second_deriv)) - (result.first_deriv * result.third_deriv);
          //change = denominator ? (-numerator / denominator) : 0.0;
          // Can't detect heading toward a maximum in Halley's method!

          // Sometimes at the first step the desired coefficient change is huge;
          //   so huge that on the second iteration the exponential term results in NAN
          if (!std::isfinite (change))
            change = 0.0;

          // Stop rare cases where a huge initial step is taken
          if (change > master.max_coeff_step)
            change = master.max_coeff_step;
          else if (change < -master.max_coeff_step)
            change = -master.max_coeff_step;

          // Early exit if outside permitted range & are continuing to move further out
          if (dFs >= master.max_coeff_step && change > 0.0) {
            dFs = master.max_coeff_step;
            change = 0.0;
          } else if (dFs <= -master.max_coeff_step && change < 0.0) {
            dFs = -master.max_coeff_step;
            change = 0.0;
          } else {
            dFs += change;
          }

        } while ((++iter < 100) && (std::abs (change) > 0.001));

#ifdef SIFT2_COEFF_OPTIMISER_DEBUG
        iter_count += iter;
#endif

        local_sum_costs += line_search_functor (0.0);

        return dFs;
      }



      }
    }
  }
}



