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




      template <>
      bool RegularisationCalculatorAbsolute<reg_basis_t::STREAMLINE, reg_fn_t::COEFF>::operator() (const SIFT::TrackIndexRange& range)
      {
        for (auto track_index : range) {
          const value_type coefficient = master.coefficients[track_index];
          local_sum += reg_coeff (coefficient);
        }
        return true;
      }
      template <>
      bool RegularisationCalculatorAbsolute<reg_basis_t::STREAMLINE, reg_fn_t::FACTOR>::operator() (const SIFT::TrackIndexRange& range)
      {
        for (auto track_index : range) {
          const value_type coefficient = master.coefficients[track_index];
          local_sum += reg_factor (coefficient);
        }
        return true;
      }
      template <>
      bool RegularisationCalculatorAbsolute<reg_basis_t::STREAMLINE, reg_fn_t::GAMMA>::operator() (const SIFT::TrackIndexRange& range)
      {
        for (auto track_index : range) {
          const value_type coefficient = master.coefficients[track_index];
          local_sum += reg_gamma (WeightingCoeffAndFactor::from_coeff (coefficient));
        }
        return true;
      }



      template <>
      bool RegularisationCalculatorAbsolute<reg_basis_t::FIXEL, reg_fn_t::COEFF>::operator() (const SIFT::TrackIndexRange& range)
      {
        for (auto track_index : range) {
          const value_type coefficient = master.coefficients[track_index];
          const SIFT::TrackContribution& this_contribution (*(master.contributions[track_index]));
          const value_type contribution_multiplier = 1.0 / this_contribution.get_total_contribution();
          value_type streamline_sum = value_type(0.0);
          for (size_t j = 0; j != this_contribution.dim(); ++j) {
            const TckFactor::Fixel fixel (master, this_contribution[j].get_fixel_index());
            const value_type fixel_coeff_cost = SIFT2::reg_coeff (coefficient, fixel.mean_coeff());
            streamline_sum += fixel.weight() * this_contribution[j].get_length() * contribution_multiplier * fixel_coeff_cost;
          }
          local_sum += streamline_sum;
        }
        return true;
      }
      template <>
      bool RegularisationCalculatorAbsolute<reg_basis_t::FIXEL, reg_fn_t::FACTOR>::operator() (const SIFT::TrackIndexRange& range)
      {
        for (auto track_index : range) {
          const value_type coefficient = master.coefficients[track_index];
          const SIFT::TrackContribution& this_contribution (*(master.contributions[track_index]));
          const value_type contribution_multiplier = 1.0 / this_contribution.get_total_contribution();
          value_type streamline_sum = value_type(0.0);
          for (size_t j = 0; j != this_contribution.dim(); ++j) {
            const TckFactor::Fixel fixel (master, this_contribution[j].get_fixel_index());
            const value_type fixel_coeff_cost = SIFT2::reg_factor (coefficient, fixel.mean_coeff());
            streamline_sum += fixel.weight() * this_contribution[j].get_length() * contribution_multiplier * fixel_coeff_cost;
          }
          local_sum += streamline_sum;
        }
        return true;
      }
      template <>
      bool RegularisationCalculatorAbsolute<reg_basis_t::FIXEL, reg_fn_t::GAMMA>::operator() (const SIFT::TrackIndexRange& range)
      {
        for (auto track_index : range) {
          const value_type coefficient = master.coefficients[track_index];
          WeightingCoeffAndFactor wcf = WeightingCoeffAndFactor::from_coeff (coefficient);
          const SIFT::TrackContribution& this_contribution (*(master.contributions[track_index]));
          const value_type contribution_multiplier = 1.0 / this_contribution.get_total_contribution();
          value_type streamline_sum = value_type(0.0);
          for (size_t j = 0; j != this_contribution.dim(); ++j) {
            const TckFactor::Fixel fixel (master, this_contribution[j].get_fixel_index());
            const value_type fixel_coeff_cost = SIFT2::reg_gamma (wcf, WeightingCoeffAndFactor::from_coeff (fixel.mean_coeff()));
            streamline_sum += fixel.weight() * this_contribution[j].get_length() * contribution_multiplier * fixel_coeff_cost;
          }
          local_sum += streamline_sum;
        }
        return true;
      }






      template <>
      bool RegularisationCalculatorDifferential<reg_basis_t::STREAMLINE>::operator() (const SIFT::TrackIndexRange& range)
      {
        for (auto track_index : range) {
          const value_type delta = master.deltas[track_index];
          local_sum += reg_delta (delta);
        }
        return true;
      }
      template <>
      bool RegularisationCalculatorDifferential<reg_basis_t::FIXEL>::operator() (const SIFT::TrackIndexRange& range)
      {
        for (auto track_index : range) {
          const value_type delta = master.deltas[track_index];
          const SIFT::TrackContribution& this_contribution (*(master.contributions[track_index]));
          const value_type contribution_multiplier = 1.0 / this_contribution.get_total_contribution();
          value_type streamline_sum = value_type(0.0);
          for (size_t j = 0; j != this_contribution.dim(); ++j) {
            const TckFactor::Fixel fixel (master, this_contribution[j].get_fixel_index());
            const value_type fixel_cost = SIFT2::reg_delta (delta, fixel.mean_delta());
            streamline_sum += fixel.weight() * this_contribution[j].get_length() * contribution_multiplier * fixel_cost;
          }
          // TODO Should contribution to overall regularisation be modulated by streamline weighting factor?
          local_sum += WeightingCoeffAndFactor::from_coeff (master.coefficients[track_index]).factor() * streamline_sum;
        }
        return true;
      }






      }
    }
  }
}



