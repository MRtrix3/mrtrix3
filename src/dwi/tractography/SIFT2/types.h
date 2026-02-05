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

#ifndef __dwi_tractography_sift2_types_h__
#define __dwi_tractography_sift2_types_h__


#include "dwi/tractography/SIFT/types.h"



namespace MR {
  namespace DWI {
    namespace Tractography {
      namespace SIFT2 {



      using value_type = SIFT::value_type;



      enum class operation_mode_t { ABSOLUTE, DIFFERENTIAL };



      // Class to track both exponential coeff and weighting factor during absolute SIFT2 calculations
      // In part this will assist to distinguish between absolute and differential calculations,
      //   as the former will use this class whereas the latter will just be a floating point number
      class WeightingCoeffAndFactor
      {
        public:
          static WeightingCoeffAndFactor from_coeff (const value_type coeff)
          {
            if (std::isnan(coeff))
              return WeightingCoeffAndFactor (coeff, coeff);
            if (coeff == -std::numeric_limits<value_type>::infinity())
              return WeightingCoeffAndFactor (coeff, value_type(0));
            assert (std::isfinite(coeff)); // Catch any positive infinities
            return WeightingCoeffAndFactor (coeff, std::exp (coeff));
          }
          static WeightingCoeffAndFactor from_factor (const value_type factor)
          {
            if (std::isnan(factor))
              return WeightingCoeffAndFactor (factor, factor);
            if (factor == value_type(0))
              return WeightingCoeffAndFactor (-std::numeric_limits<value_type>::infinity(), value_type(0));
            assert (std::isfinite (factor) && factor > value_type(0));
            return WeightingCoeffAndFactor (std::log (factor), factor);
          }
          WeightingCoeffAndFactor (const WeightingCoeffAndFactor& that) :
              _coeff (that.coeff()),
              _factor (that.factor()) { }

          value_type coeff() const { return _coeff; }
          value_type factor() const { return _factor; }

        private:
          const value_type _coeff, _factor;
          WeightingCoeffAndFactor (const value_type coeff, const value_type factor) :
              _coeff (coeff),
              _factor (factor) { }
      };



      class DifferentialWCF : public WeightingCoeffAndFactor {
        public:
          static DifferentialWCF from_deltacoeff(const WeightingCoeffAndFactor &abs, const value_type delta_coeff) {
            return DifferentialWCF(abs, delta_coeff, coeff2factor(abs, delta_coeff));
          }
          static DifferentialWCF from_coeffs(const value_type abs_coeff, const value_type delta_coeff) {
            const WeightingCoeffAndFactor abs (WeightingCoeffAndFactor::from_coeff(abs_coeff));
            return DifferentialWCF(abs, delta_coeff, coeff2factor(abs, delta_coeff));
          }

          value_type delta_coeff() const { return _delta_coeff; }
          value_type delta_factor() const { return _delta_factor; }
          WeightingCoeffAndFactor absolute() const { return WeightingCoeffAndFactor(*this); }
        private:
          const value_type _delta_coeff, _delta_factor;
          DifferentialWCF (const WeightingCoeffAndFactor &abs, const value_type delta_coeff, const value_type delta_factor) :
              WeightingCoeffAndFactor(abs),
              _delta_coeff(delta_coeff),
              _delta_factor(delta_factor) { }
          static value_type coeff2factor(const WeightingCoeffAndFactor &abs, const value_type delta_coeff) {
            return (delta_coeff * abs.factor());
          }

      };






      }
    }
  }
}


#endif

