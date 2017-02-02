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


#ifndef __dwi_tractography_sift2_regularisation_h__
#define __dwi_tractography_sift2_regularisation_h__


#include "dwi/tractography/SIFT2/line_search.h"


namespace MR
{
  namespace DWI
  {
    namespace Tractography
    {
      namespace SIFT2
      {



      template <typename value_type>
      inline value_type tvreg (const value_type coeff, const value_type base)
      {
        return ((coeff <= base) ?
            (Math::pow2(coeff - base)) :
            (Math::pow2(std::exp(coeff) - std::exp(base))));
      }
      template <typename value_type>
      inline value_type dtvreg_dcoeff (const value_type coeff, const value_type base)
      {
        return ((coeff <= base) ?
            (value_type(2.0) * (coeff - base)) :
            (value_type(2.0) * std::exp(coeff) * (std::exp(coeff) - std::exp(base))));
      }
      template <typename value_type>
      inline value_type d2tvreg_dcoeff2 (const value_type coeff, const value_type base)
      {
        return ((coeff <= base) ?
            (value_type(2.0)) :
            (value_type(2.0) * std::exp(coeff) * ((value_type(2.0) * std::exp(coeff)) - std::exp(base))));
      }
      template <typename value_type>
      inline value_type d3tvreg_dcoeff3 (const value_type coeff, const value_type base)
      {
        return ((coeff <= base) ?
            (value_type(0.0)) :
            (value_type(2.0) * std::exp(coeff) * ((value_type(4.0) * std::exp(coeff)) - std::exp(base))));
      }
      // A convenience function for LineSearchFunctor to reduce Math::exp() calls
      inline void dxtvreg_dcoeffx (LineSearchFunctor::Result& result, const double coeff, const double expcoeff, const double multiplier, const double base, const double expbase)
      {
        if (coeff <= base) {
          result.cost         += multiplier * Math::pow2 (coeff - base);
          result.first_deriv  += multiplier * 2.0 * (coeff - base);
          result.second_deriv += multiplier * 2.0;
        } else {
          result.cost         += multiplier * Math::pow2 (expcoeff - expbase);
          result.first_deriv  += multiplier * 2.0 * expcoeff * (expcoeff - expbase);
          result.second_deriv += multiplier * 2.0 * expcoeff * ((2.0*expcoeff) - expbase);
          result.third_deriv  += multiplier * 2.0 * expcoeff * ((4.0*expcoeff) - expbase);
        }
      }



      }
    }
  }
}


#endif


