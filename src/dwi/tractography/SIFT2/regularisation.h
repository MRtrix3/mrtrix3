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


