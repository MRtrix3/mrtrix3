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

#include "dwi/tractography/SIFT2/coeff_optimiser.h"
#include "dwi/tractography/SIFT2/cost_and_derivatives.h"
#include "dwi/tractography/SIFT2/line_search.h"
#include "dwi/tractography/SIFT2/tckfactor.h"




namespace MR {
  namespace DWI {
    namespace Tractography {
      namespace SIFT2 {





      CoefficientOptimiserBase::CoefficientOptimiserBase (TckFactor& tckfactor, StreamlineStats& step_stats, StreamlineStats& coefficient_stats, SIFT::track_t& participating_streamlines, BitSet& fixels_to_exclude, value_type& sum_costs) :
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
            participating_streamlines (participating_streamlines),
            fixels_to_exclude (fixels_to_exclude),
            sum_costs (sum_costs),
            local_stats_steps (),
            local_stats_coefficients (),
            local_participation_count (0),
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
            participating_streamlines (that.participating_streamlines),
            fixels_to_exclude (that.fixels_to_exclude),
            sum_costs (that.sum_costs),
            local_stats_steps (),
            local_stats_coefficients (),
            local_participation_count (0),
            local_sum_costs (0.0) { }



      CoefficientOptimiserBase::~CoefficientOptimiserBase()
      {
        std::lock_guard<std::mutex> lock (master.mutex);
#ifdef SIFT2_COEFF_OPTIMISER_DEBUG
        fprintf (stderr, "%ld of %ld initial searches failed, %ld in wrong direction, %ld steps truncated, %ld coefficients truncated\n", failed, total, wrong_dir, step_truncated, coeff_truncated);
#endif
        step_stats += local_stats_steps;
        coefficient_stats += local_stats_coefficients;
        participating_streamlines += local_participation_count;
        for (auto i : local_streamlines_to_exclude)
          master.mask_absolute[i] = false;
        for (auto f : local_fixels_to_exclude)
          fixels_to_exclude[f] = true;
        sum_costs += local_sum_costs;
      }



      bool CoefficientOptimiserBase::operator() (const SIFT::TrackIndexRange& range)
      {

        for (auto track_index : range) {
          if (!master.mask_absolute[track_index])
            continue;

          value_type dFs = get_coeff_change (track_index);

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

          const value_type old_coefficient = master.coefficients[track_index];
          value_type new_coefficient = old_coefficient + dFs;
          if (new_coefficient < master.min_coeff) {
            new_coefficient = -std::numeric_limits<value_type>::infinity();
            dFs = master.min_coeff - old_coefficient;
            local_streamlines_to_exclude.push_back(track_index);
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
            ++local_participation_count;

#ifdef STREAMLINE_OF_INTEREST
          if (track_index == STREAMLINE_OF_INTEREST) {

            {
              std::ofstream out ("soi_general.csv", std::ios_base::out | std::ios_base::app | std::ios_base::ate);
              out << old_coefficient << "," << this_projected_step << "," << dFs << "," << new_coefficient << "\n";
            }
            {
              std::ofstream out ("soi_ls_coeff.csv", std::ios_base::out | std::ios_base::app | std::ios_base::ate);
              LineSearchFunctorAbsolute line_search_functor (track_index, master, this_projected_step);
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



      SIFT::value_type CoefficientOptimiserBase::do_fixel_exclusion (const SIFT::track_t track_index)
      {
        const SIFT::TrackContribution& this_contribution (*(master.contributions[track_index]));

        // Task 1: Identify the fixel that should be excluded
        size_t index_to_exclude = 0.0;
        value_type cost_to_exclude = 0.0;

        for (size_t j = 0; j != this_contribution.dim(); ++j) {
          const MR::Fixel::index_type fixel_index = this_contribution[j].get_fixel_index();
          const float length = this_contribution[j].get_length();
          const TckFactor::Fixel fixel (master, fixel_index);
          if (!fixel.excluded() && (fixel.get_diff (mu) < 0.0)) {

            const value_type this_cost = fixel.get_cost (mu) * length / fixel.orig_TD();
            if (this_cost > cost_to_exclude) {
              cost_to_exclude = this_cost;
              index_to_exclude = fixel_index;
            }

          }
        }

        if (index_to_exclude)
          local_fixels_to_exclude.push_back(index_to_exclude);
        else
          return 0.0;

        // Task 2: Calculate a new coefficient for this streamline
        value_type weighted_sum = 0.0, sum_weights = 0.0;

        for (size_t j = 0; j != this_contribution.dim(); ++j) {
          const MR::Fixel::index_type fixel_index = this_contribution[j].get_fixel_index();
          const float length = this_contribution[j].get_length();
          const TckFactor::Fixel fixel (master, fixel_index);
          if (!fixel.excluded() && (fixel_index != index_to_exclude)) {
            weighted_sum += length * fixel.weight() * fixel.mean_coeff();
            sum_weights  += length * fixel.weight();
          }
        }

        return (sum_weights ? (weighted_sum / sum_weights) : 0.0);
      }











/*
      CoefficientOptimiserGSS::CoefficientOptimiserGSS (TckFactor& tckfactor, StreamlineStats& step_stats, StreamlineStats& coefficient_stats, SIFT::track_t& participating_streamlines, BitSet& fixels_to_exclude, value_type& sum_costs) :
            CoefficientOptimiserBase (tckfactor, step_stats, coefficient_stats, participating_streamlines, fixels_to_exclude, sum_costs) { }

      CoefficientOptimiserGSS::CoefficientOptimiserGSS (const CoefficientOptimiserGSS& that) :
            CoefficientOptimiserBase (that) { }

      SIFT::value_type CoefficientOptimiserGSS::get_coeff_change (const SIFT::track_t track_index) const
      {
        LineSearchFunctorAbsolute line_search_functor (track_index, master);
        const value_type dFs = Math::golden_section_search (line_search_functor, std::string(""), -master.max_coeff_step, 0.0, master.max_coeff_step, 0.001 / (2.0 * master.max_coeff_step));
        const value_type cost = line_search_functor (dFs);
        // The Golden Section Search doesn't check the endpoints; do that here
        // TODO Once GSS is turned into a functor, add this capability to it
        if ((dFs > 0.99 * master.max_coeff_step) && (line_search_functor (master.max_coeff_step) < cost))
          return master.max_coeff_step;
        if ((dFs < -0.99 * master.max_coeff_step) && (line_search_functor (-master.max_coeff_step) < cost))
          return -master.max_coeff_step;
        return dFs;
      }
*/





/*
      CoefficientOptimiserQLS::CoefficientOptimiserQLS (TckFactor& tckfactor, StreamlineStats& step_stats, StreamlineStats& coefficient_stats, SIFT::track_t& participating_streamlines, BitSet& fixels_to_exclude, value_type& sum_costs) :
            CoefficientOptimiserBase (tckfactor, step_stats, coefficient_stats, participating_streamlines, fixels_to_exclude, sum_costs),
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

      SIFT::value_type CoefficientOptimiserQLS::get_coeff_change (const SIFT::track_t track_index) const
      {
        LineSearchFunctorAbsolute line_search_functor (track_index, master);

        value_type dFs = qls (line_search_functor);
#ifdef SIFT2_COEFF_OPTIMISER_DEBUG
        ++total;
#endif
        if (!std::isfinite (dFs)) {
          dFs = Math::golden_section_search (line_search_functor, std::string(""), -master.max_coeff_step, 0.0, master.max_coeff_step, 0.001 / (2.0 * master.max_coeff_step));
          value_type cost = line_search_functor (dFs);
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
*/







      CoefficientOptimiserIterative::CoefficientOptimiserIterative (TckFactor& tckfactor, StreamlineStats& step_stats, StreamlineStats& coefficient_stats, SIFT::track_t& participating_streamlines, BitSet& fixels_to_exclude, value_type& sum_costs) :
            CoefficientOptimiserBase (tckfactor, step_stats, coefficient_stats, participating_streamlines, fixels_to_exclude, sum_costs)
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

      SIFT::value_type CoefficientOptimiserIterative::get_coeff_change (const SIFT::track_t track_index) const
      {

        LineSearchFunctorAbsolute line_search_functor (track_index, master);

        value_type dFs = 0.0;
        value_type change = 0.0;
        size_t iter = 0;
        do {

          // TODO For the sake of getting things working initially,
          //   let's branch here based on the regularisation;
          //   once things are compiling again,
          //   we can then move the template realisation further out for optimisation
          CostAndDerivatives cost_and_derivatives;
          switch (master.reg_basis_abs) {
            case reg_basis_t::STREAMLINE: {
              switch (master.reg_fn_abs) {
                case reg_fn_abs_t::COEFF:  cost_and_derivatives = line_search_functor.get<reg_basis_t::STREAMLINE, reg_fn_abs_t::COEFF>  (dFs); break;
                case reg_fn_abs_t::FACTOR: cost_and_derivatives = line_search_functor.get<reg_basis_t::STREAMLINE, reg_fn_abs_t::FACTOR> (dFs); break;
                case reg_fn_abs_t::GAMMA:  cost_and_derivatives = line_search_functor.get<reg_basis_t::STREAMLINE, reg_fn_abs_t::GAMMA>  (dFs); break;
              }
            }
            break;
            case reg_basis_t::FIXEL: {
              switch (master.reg_fn_abs) {
                case reg_fn_abs_t::COEFF:  cost_and_derivatives = line_search_functor.get<reg_basis_t::FIXEL, reg_fn_abs_t::COEFF>  (dFs); break;
                case reg_fn_abs_t::FACTOR: cost_and_derivatives = line_search_functor.get<reg_basis_t::FIXEL, reg_fn_abs_t::FACTOR> (dFs); break;
                case reg_fn_abs_t::GAMMA:  cost_and_derivatives = line_search_functor.get<reg_basis_t::FIXEL, reg_fn_abs_t::GAMMA>  (dFs); break;
              }
            }
            break;
          }

          // Newton update
          change = cost_and_derivatives.second_deriv ? (-cost_and_derivatives.first_deriv / cost_and_derivatives.second_deriv) : 0.0;
          if (cost_and_derivatives.second_deriv < 0.0)
            change = -change;

          // Halley update
          //const value_type numerator   = 2.0 * cost_and_derivatives.first_deriv * cost_and_derivatives.second_deriv;
          //const value_type denominator = (2.0 * Math::pow2 (cost_and_derivatives.second_deriv)) - (cost_and_derivatives.first_deriv * cost_and_derivatives.third_deriv);
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

        } while ((++iter < 100) && (abs (change) > 0.001));

#ifdef SIFT2_COEFF_OPTIMISER_DEBUG
        iter_count += iter;
#endif

        // TODO Remove once template instantiation is moved out
        switch (master.reg_basis_abs) {
          case reg_basis_t::STREAMLINE: {
            switch (master.reg_fn_abs) {
              case reg_fn_abs_t::COEFF:  local_sum_costs += line_search_functor.operator()<reg_basis_t::STREAMLINE, reg_fn_abs_t::COEFF>  (dFs);
              case reg_fn_abs_t::FACTOR: local_sum_costs += line_search_functor.operator()<reg_basis_t::STREAMLINE, reg_fn_abs_t::FACTOR> (dFs); break;
              case reg_fn_abs_t::GAMMA:  local_sum_costs += line_search_functor.operator()<reg_basis_t::STREAMLINE, reg_fn_abs_t::GAMMA>  (dFs); break;
            }
          }
          break;
          case reg_basis_t::FIXEL: {
            switch (master.reg_fn_abs) {
              case reg_fn_abs_t::COEFF:  local_sum_costs += line_search_functor.operator()<reg_basis_t::FIXEL, reg_fn_abs_t::COEFF>  (dFs); break;
              case reg_fn_abs_t::FACTOR: local_sum_costs += line_search_functor.operator()<reg_basis_t::FIXEL, reg_fn_abs_t::FACTOR> (dFs); break;
              case reg_fn_abs_t::GAMMA:  local_sum_costs += line_search_functor.operator()<reg_basis_t::FIXEL, reg_fn_abs_t::GAMMA>  (dFs); break;
            }
          }
          break;
        }

        return dFs;
      }




      template <reg_basis_t RegBasis, reg_fn_abs_t RegFn>
      SIFT::value_type CoefficientOptimiserBBGD<RegBasis, RegFn>::get_coeff_change (const SIFT::track_t track_index) const
      {
        // TODO Manually calculate gradient since:
        // - We don't want to do the fractional attribution of fixel-wise cost to the intersecting streamlines
        // - We are not including the correlation term
        // - Avoid the computational expense of calculating higher-order derivatives

        const SIFT::TrackContribution& this_contribution (*(master.contributions[track_index]));
        const value_type reg_multiplier = master.reg_multiplier_abs * (RegBasis == reg_basis_t::FIXEL ? (1.0 / this_contribution.get_total_contribution()) : 1.0);
        const WeightingCoeffAndFactor WCF = WeightingCoeffAndFactor::from_coeff (master.coefficients[track_index]);
        value_type gradient = 0.0;
        for (size_t j = 0; j != this_contribution.dim(); ++j) {
          const MR::Fixel::index_type fixel_index = this_contribution[j].get_fixel_index();
          const float length = this_contribution[j].get_length();
          const TckFactor::Fixel fixel (master, fixel_index);
          if (!fixel.excluded()) {

            // Data term
            // TD = (orig_TD - length) + (length * e^coeff)
            // dTD/dcoeff = length * e^coeff (= length * streamline weighting factor)
            // diff = mu.TD - FD
            // cost = fixel_weight * diff^2
            // dcost/dTD = dcost/ddiff * ddiff/dTD = (2.0 * fixel_weight * diff) * mu
            // dcost/dcoeff = dcost/dTD * dTD/dcoeff = (2.0 * fixel_weight * diff * mu) * length * factor
            gradient += 2.0 * fixel.weight() * fixel.get_diff (mu) * mu * length * WCF.factor();

            // Regularisation term when fixel-wise
            if (RegBasis == reg_basis_t::FIXEL) {
              // Need to consider fraction of fixel regularisation going to that streamline, and the term itself
              gradient += reg_multiplier * fixel.weight() * (length / fixel.orig_TD()) * SIFT2::dreg_dcoeff<RegFn> (WCF, WeightingCoeffAndFactor::from_coeff (fixel.mean_coeff()));
            }

          }
        }

        // Regularisation term when streamline-wise
        if (RegBasis == reg_basis_t::STREAMLINE)
          gradient += reg_multiplier * SIFT2::dreg_dcoeff<RegFn> (WCF);

        gradients[track_index] = gradient;

        // TODO Verify gradient calculations using finite difference?
        // - On a per-streamline basis
        // - Using the whole image, perturbing just that one streamline weight?

        return (-step_size * gradient);
      }
      template SIFT::value_type CoefficientOptimiserBBGD<reg_basis_t::STREAMLINE, reg_fn_abs_t::COEFF>::get_coeff_change (const SIFT::track_t) const;
      template SIFT::value_type CoefficientOptimiserBBGD<reg_basis_t::STREAMLINE, reg_fn_abs_t::FACTOR>::get_coeff_change (const SIFT::track_t) const;
      template SIFT::value_type CoefficientOptimiserBBGD<reg_basis_t::STREAMLINE, reg_fn_abs_t::GAMMA>::get_coeff_change (const SIFT::track_t) const;
      template SIFT::value_type CoefficientOptimiserBBGD<reg_basis_t::FIXEL, reg_fn_abs_t::COEFF>::get_coeff_change (const SIFT::track_t) const;
      template SIFT::value_type CoefficientOptimiserBBGD<reg_basis_t::FIXEL, reg_fn_abs_t::FACTOR>::get_coeff_change (const SIFT::track_t) const;
      template SIFT::value_type CoefficientOptimiserBBGD<reg_basis_t::FIXEL, reg_fn_abs_t::GAMMA>::get_coeff_change (const SIFT::track_t) const;







      DeltaOptimiserIterative::DeltaOptimiserIterative (TckFactor& tckfactor, StreamlineStats& step_stats, StreamlineStats& delta_stats, value_type& sum_costs) :
          master (tckfactor),
          mu (tckfactor.mu()),
#ifdef SIFT2_DIFFERENTIAL_OPTIMISER_DEBUG
          total (0),
          failed (0),
          wrong_dir (0),
          step_truncated (0),
          delta_truncated (0),
#endif
          step_stats (step_stats),
          delta_stats (delta_stats),
          sum_costs (sum_costs),
          local_stats_steps (),
          local_stats_deltas (),
          local_sum_costs (0.0) { }



      DeltaOptimiserIterative::~DeltaOptimiserIterative()
      {
        std::lock_guard<std::mutex> lock (master.mutex);
#ifdef SIFT2_DIFFERENTIAL_OPTIMISER_DEBUG
        fprintf (stderr, "%ld of %ld initial searches failed, %ld in wrong direction, %ld steps truncated, %ld deltas truncated\n", failed, total, wrong_dir, step_truncated, delta_truncated);
#endif
        step_stats += local_stats_steps;
        delta_stats += local_stats_deltas;
        sum_costs += local_sum_costs;
      }




      bool DeltaOptimiserIterative::operator() (const SIFT::TrackIndexRange& range)
      {

        for (auto track_index : range) {

          if (!master.mask_differential[track_index])
            continue;

          value_type dDelta = (*this) (track_index);

#ifdef SIFT2_COEFF_OPTIMISER_DEBUG
          ++total;
#endif

          if (dDelta >= master.max_deltacoeff_step) {
            dDelta = master.max_deltacoeff_step;
#ifdef SIFT2_COEFF_OPTIMISER_DEBUG
            ++step_truncated;
#endif
          } else if (dDelta <= -master.max_deltacoeff_step) {
            dDelta = -master.max_deltacoeff_step;
#ifdef SIFT2_COEFF_OPTIMISER_DEBUG
            ++step_truncated;
#endif
          }

          const value_type old_deltacoeff = master.deltacoeffs[track_index];
          value_type new_deltacoeff = old_deltacoeff + dDelta;
          if (new_deltacoeff < master.min_deltacoeff) {
            new_deltacoeff = master.min_deltacoeff;
            dDelta = master.min_deltacoeff - old_deltacoeff;
#ifdef SIFT2_COEFF_OPTIMISER_DEBUG
            ++delta_truncated;
#endif
          } else if (new_deltacoeff > master.max_deltacoeff) {
            //new_deltacoeff = do_fixel_exclusion (track_index);
            new_deltacoeff = master.max_deltacoeff;
            dDelta = master.max_deltacoeff - old_deltacoeff;
#ifdef SIFT2_COEFF_OPTIMISER_DEBUG
            ++delta_truncated;
#endif
          }

          master.deltacoeffs[track_index] = new_deltacoeff;

          // Update the stats
          local_stats_steps += dDelta;
          local_stats_deltas += new_deltacoeff;

        }

        return true;
      }





      value_type DeltaOptimiserIterative::operator() (const SIFT::track_t track_index) const
      {
        LineSearchFunctorDifferential line_search_functor (track_index, master);
        const value_type delta = master.deltacoeffs[track_index];

        value_type dDelta = 0.0;
        value_type change = 0.0;
        size_t iter = 0;
        CostAndDerivatives cost_and_derivatives;
        do {
          switch (master.reg_basis_diff) {
            case reg_basis_t::STREAMLINE:
              switch (master.reg_fn_diff) {
                case reg_fn_diff_t::DELTACOEFF:  cost_and_derivatives = line_search_functor.get<reg_basis_t::STREAMLINE, reg_fn_diff_t::DELTACOEFF>  (dDelta); break;
                case reg_fn_diff_t::DUALINVBARR: cost_and_derivatives = line_search_functor.get<reg_basis_t::STREAMLINE, reg_fn_diff_t::DUALINVBARR> (dDelta); break;
              }
              break;
            case reg_basis_t::FIXEL:
              switch (master.reg_fn_diff) {
                case reg_fn_diff_t::DELTACOEFF:  cost_and_derivatives = line_search_functor.get<reg_basis_t::FIXEL, reg_fn_diff_t::DELTACOEFF>  (dDelta); break;
                case reg_fn_diff_t::DUALINVBARR: cost_and_derivatives = line_search_functor.get<reg_basis_t::FIXEL, reg_fn_diff_t::DUALINVBARR> (dDelta); break;
              }
              break;
          }

          // Newton update
          change = cost_and_derivatives.second_deriv ? (-cost_and_derivatives.first_deriv / cost_and_derivatives.second_deriv) : 0.0;
          if (cost_and_derivatives.second_deriv < 0.0)
            change = -change;

          // Halley update
          //const value_type numerator   = 2.0 * cost_and_derivatives.first_deriv * cost_and_derivatives.second_deriv;
          //const value_type denominator = (2.0 * Math::pow2 (cost_and_derivatives.second_deriv)) - (cost_and_derivatives.first_deriv * cost_and_derivatives.third_deriv);
          //change = denominator ? (-numerator / denominator) : 0.0;

          if (!std::isfinite (change))
            change = 0.0;

          if (change > master.max_deltacoeff_step)
            change = master.max_deltacoeff_step;
          else if (change < -master.max_deltacoeff_step)
            change = -master.max_deltacoeff_step;

          if (dDelta >= master.max_deltacoeff_step && change > 0.0) {
            dDelta = master.max_deltacoeff_step;
            change = 0.0;
          } else if (dDelta <= -master.max_deltacoeff_step && change < 0.0) {
            dDelta = -master.max_deltacoeff_step;
            change = 0.0;
          // Additional conditions for differential mode
          // For the asymptotic regulariser in particular,
          //   delta + dDelta + change cannot be permitted outside of range (-1, +1);
          // for the deltacoeff optimiser,
          //   delta + dDelta + change cannot be permitted outside of range [-1, +1]
          // TODO This is still in danger of having rounding errors,
          //   as dDelta is passed from this functor up to the calling function
          //   before it is added to deltacoeffs.
          //   It may be better to have this functor return both the change and the new value?
          } else if (master.reg_fn_diff == reg_fn_diff_t::DUALINVBARR) {
            if (delta + dDelta + change >= 1.0)
              change = 0.5 * (1.0 - (delta + dDelta));
            else if (delta + dDelta + change <= -1.0)
              change = -0.5 * (1.0 + (delta + dDelta));
            dDelta += change;
          } else {
            if (delta + dDelta + change >= 1.0) {
              change = 1.0 - (delta + dDelta);
              dDelta = 1.0 - delta;
            } else if (delta + dDelta + change <= -1.0) {
              change = -1.0 - (delta + dDelta);
              dDelta = -1.0 - delta;
            } else {
              dDelta += change;
            }
          }

        } while ((++iter < 100) && (abs (change) > 1e-4));

#ifdef SIFT2_DIFFERENTIAL_OPTIMISER_DEBUG
        iter_count += iter;
#endif

        switch (master.reg_basis_diff) {
          case reg_basis_t::STREAMLINE:
            switch (master.reg_fn_diff) {
              case reg_fn_diff_t::DELTACOEFF:  local_sum_costs += line_search_functor.operator()<reg_basis_t::STREAMLINE, reg_fn_diff_t::DELTACOEFF>  (dDelta); break;
              case reg_fn_diff_t::DUALINVBARR: local_sum_costs += line_search_functor.operator()<reg_basis_t::STREAMLINE, reg_fn_diff_t::DUALINVBARR> (dDelta); break;
            }
            break;
          case reg_basis_t::FIXEL:
            switch (master.reg_fn_diff) {
              case reg_fn_diff_t::DELTACOEFF:  local_sum_costs += line_search_functor.operator()<reg_basis_t::FIXEL, reg_fn_diff_t::DELTACOEFF>  (dDelta); break;
              case reg_fn_diff_t::DUALINVBARR: local_sum_costs += line_search_functor.operator()<reg_basis_t::FIXEL, reg_fn_diff_t::DUALINVBARR> (dDelta); break;
            }
            break;

        }

        return dDelta;
      }







      }
    }
  }
}



