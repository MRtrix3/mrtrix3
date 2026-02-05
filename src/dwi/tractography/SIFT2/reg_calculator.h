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





      class RegularisationCalculatorBase
      {
        public:
          using value_type = SIFT::value_type;

          RegularisationCalculatorBase (TckFactor& tckfactor, value_type& global_sum) :
              master (tckfactor),
              global_sum (global_sum),
              local_sum (value_type(0.0)) { }

          ~RegularisationCalculatorBase()
          {
            std::lock_guard<std::mutex> lock (master.mutex);
            global_sum += local_sum;
          }

          // TODO Should be possible to define this in the base class,
          //   and just execute a per-index functor for the derived class
          virtual bool operator() (const SIFT::TrackIndexRange& range) = 0;

        protected:
          TckFactor& master;
          value_type& global_sum;
          value_type local_sum;
      };



      template <reg_basis_t RegBasis, reg_fn_abs_t RegFn>
      class RegularisationCalculatorAbsolute : public RegularisationCalculatorBase
      {
        public:
          RegularisationCalculatorAbsolute (TckFactor& tckfactor, value_type& global_sum) :
              RegularisationCalculatorBase (tckfactor, global_sum) { }
          bool operator() (const SIFT::TrackIndexRange& range) override;
      };




      template <reg_basis_t RegBasis, reg_fn_diff_t RegFn>
      class RegularisationCalculatorDifferential : public RegularisationCalculatorBase
      {
        public:
          RegularisationCalculatorDifferential (TckFactor& tckfactor, value_type& global_sum) :
              RegularisationCalculatorBase (tckfactor, global_sum) { }
          bool operator() (const SIFT::TrackIndexRange& range) override;
      };




      }
    }
  }
}



#endif

