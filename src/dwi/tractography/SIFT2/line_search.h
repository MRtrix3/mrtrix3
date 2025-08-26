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

#ifndef __dwi_tractography_sift2_line_search_h__
#define __dwi_tractography_sift2_line_search_h__


#include "types.h"

#include "dwi/tractography/SIFT/track_contribution.h"
#include "dwi/tractography/SIFT/types.h"
#include "dwi/tractography/SIFT2/tckfactor.h"




namespace MR {
  namespace DWI {
    namespace Tractography {
      namespace SIFT2 {



      class TckFactor;



      // New line search functor for when per-streamline projections and per-fixel correlation terms are not calculated
      // Instead, the correlation term for the line search is derived using the TD fraction only
      class LineSearchFunctor
      {

        public:
          using value_type = SIFT::value_type;

          class Result
          {
            public:
            Result() : cost (0.0), first_deriv (0.0), second_deriv (0.0), third_deriv (0.0) { }
            Result& operator+= (const Result& that) { cost += that.cost; first_deriv += that.first_deriv; second_deriv += that.second_deriv; third_deriv += that.third_deriv; return *this; }
            Result& operator*= (const value_type i) { cost *= i; first_deriv *= i; second_deriv *= i; third_deriv *= i; return *this; }
            value_type cost, first_deriv, second_deriv, third_deriv;
            bool valid() const { return std::isfinite(cost) && std::isfinite(first_deriv) && std::isfinite(second_deriv) && std::isfinite(third_deriv); }
          };

          LineSearchFunctor (const SIFT::track_t, TckFactor&);


          // Interfaces for line searches
          Result     get        (const value_type) const;
          value_type operator() (const value_type) const;


        protected:

          // Necessary information for those fixels traversed by this streamline
          class Fixel
          {
            public:
            Fixel (const SIFT::Track_fixel_contribution&, const TckFactor::Fixel, const value_type, const value_type);
            //void set_damping (const value_type i) { dTD_dFs *= i; }
            MR::Fixel::index_type index;
            value_type length, PM, TD, cost_frac, SL_eff, dTD_dFs, meanFs, expmeanFs, FD;
          };


          const SIFT::track_t track_index;
          const value_type mu;
          const value_type Fs;
          const value_type reg_tik, reg_tv;

          vector<Fixel> fixels;

      };





      }
    }
  }
}


#endif

