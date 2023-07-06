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

#ifndef __dwi_tractography_sift2_reg_calculator_h__
#define __dwi_tractography_sift2_reg_calculator_h__


#include "dwi/tractography/SIFT/track_index_range.h"
#include "dwi/tractography/SIFT/types.h"

#include "dwi/tractography/SIFT2/regularisation.h"
#include "dwi/tractography/SIFT2/tckfactor.h"


namespace MR {
  namespace DWI {
    namespace Tractography {
      namespace SIFT2 {


      // TODO Try implementing alternative templated version completely in parallel
      // Going to have to change multiple things in multiple places before it can all compile together
      template <reg_basis_t RegBasis, reg_fn_t RegFn>
      class RegularisationCalculator
      {
        public:
          using value_type = SIFT::value_type;

          RegularisationCalculator (TckFactor& tckfactor, value_type& global_sum) :
            master (tckfactor),
            global_sum (global_sum),
            local_sum (value_type(0.0)) { }

          ~RegularisationCalculator()
          {
            std::lock_guard<std::mutex> lock (master.mutex);
            global_sum += local_sum;
          }

          bool operator() (const SIFT::TrackIndexRange& range);

        private:
          TckFactor& master;
          value_type& global_sum;
          value_type local_sum;
      };

      // TODO Not working
      // Is this a partial template specialisation problem?
      // TODO Try to get working
      // template <reg_fn_t RegFn>
      // bool RegularisationCalculator<reg_basis_t::STREAMLINE, RegFn>::operator() (const SIFT::TrackIndexRange& range)
      // {
      //   for (auto track_index : range) {
      //     const value_type coefficient = master.coefficients[track_index];
      //     local_sum += reg<RegFn> (coefficient);
      //   }
      //   return true;
      // }


      // template <reg_fn_t RegFn>
      // bool RegularisationCalculator<reg_basis_t::FIXEL, RegFn>::operator() (const SIFT::TrackIndexRange& range)
      // {
      //   for (auto track_index : range) {
      //     const value_type coefficient = master.coefficients[track_index];
      //     const SIFT::TrackContribution& this_contribution (*(master.contributions[track_index]));
      //     const value_type contribution_multiplier = 1.0 / this_contribution.get_total_contribution();
      //     value_type streamline_sum = value_type(0.0);
      //     for (size_t j = 0; j != this_contribution.dim(); ++j) {
      //       const TckFactor::Fixel fixel (master, this_contribution[j].get_fixel_index());
      //       const value_type fixel_coeff_cost = SIFT2::reg<RegFn> (coefficient, fixel.mean_coeff());
      //       streamline_sum += fixel.weight() * this_contribution[j].get_length() * contribution_multiplier * fixel_coeff_cost;
      //     }
      //     local_sum += streamline_sum;
      //   }
      //   return true;
      // }



      }
    }
  }
}



#endif

