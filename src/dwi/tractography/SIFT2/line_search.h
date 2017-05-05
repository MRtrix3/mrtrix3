/* Copyright (c) 2008-2017 the MRtrix3 contributors
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


#ifndef __dwi_tractography_sift2_line_search_h__
#define __dwi_tractography_sift2_line_search_h__


#include <vector>

#include "dwi/tractography/SIFT/track_contribution.h"
#include "dwi/tractography/SIFT/types.h"



namespace MR {
  namespace DWI {
    namespace Tractography {
      namespace SIFT2 {



      class TckFactor;



      // New line search functor for when per-streamline projections and per-fixel correlation terms are not calculated
      // Instead, the correlation term for the line search is derived using the TD fraction only
      class LineSearchFunctor
      { MEMALIGN(LineSearchFunctor)

        public:

          class Result
          { NOMEMALIGN
            public:
            Result() : cost (0.0), first_deriv (0.0), second_deriv (0.0), third_deriv (0.0) { }
            Result& operator+= (const Result& that) { cost += that.cost; first_deriv += that.first_deriv; second_deriv += that.second_deriv; third_deriv += that.third_deriv; return *this; }
            Result& operator*= (const double i) { cost *= i; first_deriv *= i; second_deriv *= i; third_deriv *= i; return *this; }
            double cost, first_deriv, second_deriv, third_deriv;
            bool valid() const { return std::isfinite(cost) && std::isfinite(first_deriv) && std::isfinite(second_deriv) && std::isfinite(third_deriv); }
          };

          LineSearchFunctor (const SIFT::track_t, const TckFactor&);


          // Interfaces for line searches
          Result get        (const double) const;
          double operator() (const double) const;


        protected:

          // Necessary information for those fixels traversed by this streamline
          class Fixel
          { NOMEMALIGN
            public:
            Fixel (const SIFT::Track_fixel_contribution&, const TckFactor&, const double, const double);
            //void set_damping (const double i) { dTD_dFs *= i; }
            uint32_t index;
            double length, PM, TD, cost_frac, SL_eff, dTD_dFs, meanFs, expmeanFs, FOD;
          };


          const SIFT::track_t track_index;
          const double mu;
          const double Fs;
          const double reg_tik, reg_tv;

          vector<Fixel> fixels;

      };





      }
    }
  }
}


#endif

