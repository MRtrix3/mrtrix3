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

#ifndef __dwi_tractography_sift2_cost_and_derivatives_h__
#define __dwi_tractography_sift2_cost_and_derivatives_h__


#include "dwi/tractography/SIFT/types.h"


namespace MR {
  namespace DWI {
    namespace Tractography {
      namespace SIFT2 {


        class CostAndDerivatives
        {
          public:
            using value_type = SIFT::value_type;
            CostAndDerivatives() :
                cost (value_type(0.0)),
                first_deriv (value_type(0.0)),
                second_deriv (value_type(0.0)),
                third_deriv (value_type(0.0)) { }
            CostAndDerivatives (const CostAndDerivatives& data, const CostAndDerivatives& reg) :
                cost (data.cost + reg.cost),
                first_deriv (data.first_deriv + reg.first_deriv),
                second_deriv (data.second_deriv + reg.second_deriv),
                third_deriv (data.third_deriv + reg.third_deriv) { }
          CostAndDerivatives& operator+= (const CostAndDerivatives& that)
          {
            cost += that.cost;
            first_deriv += that.first_deriv;
            second_deriv += that.second_deriv;
            third_deriv += that.third_deriv;
            return *this;
          }
          CostAndDerivatives& operator*= (const value_type i)
          {
            cost *= i;
            first_deriv *= i;
            second_deriv *= i;
            third_deriv *= i;
            return *this;
          }
          bool valid() const
          {
            return std::isfinite(cost) &&
                   std::isfinite(first_deriv) &&
                   std::isfinite(second_deriv) &&
                   std::isfinite(third_deriv);
          }
          value_type cost, first_deriv, second_deriv, third_deriv;
        };

        FORCE_INLINE std::ostream &operator<<(std::ostream &stream, const CostAndDerivatives &data) {
          stream << "[" << data.cost << ", " << data.first_deriv << ", " << data.second_deriv << ", " << data.third_deriv << "]";
          return stream;
        }



      }
    }
  }
}


#endif
