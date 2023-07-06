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

#ifndef __dwi_tractography_sift2_regularisation_h__
#define __dwi_tractography_sift2_regularisation_h__



#include "app.h"
#include "math/math.h"
#include "dwi/tractography/SIFT2/cost_and_derivatives.h"
#include "dwi/tractography/SIFT/types.h"


// TODO Consider SIFT2::Regularisation namespace
// - Some function renaming
// - Move calculator class from external file reg_calculator.*



namespace MR
{
  namespace DWI
  {
    namespace Tractography
    {
      namespace SIFT2
      {



      using value_type = SIFT::value_type;






      // New type definitions for regularisation
      // Instead of "tik" and "aTV" regularisation,
      //   will instead have a singular form of regularisation applied in any invocation;
      //   that regularisation will be defined by:
      // - Is it applied on a per-streamline or a per-fixel basis?
      //   (with the latter being with respect to the fixel-wise mean coefficient)
      // - What functional form does the regularisation function take?
      //   1. Penalises streamline weight
      //   2. Penalises streamline coefficient
      //   3. Utilise reg_gamma function
      //
      // Ideally, for computational performance, these would be templated out,
      //   since they may be involved in derivative calculations in deep line search loops

      // TODO Experiment with non-strong enums,
      //   since we want to be able to template by these
      enum reg_basis_t { STREAMLINE, FIXEL };
      enum reg_fn_t { WEIGHT, COEFF, GAMMA };

      extern const char* reg_basis_choices[];
      extern const char* reg_fn_choices[];

      extern App::OptionGroup RegularisationOptions;

      constexpr default_type regularisation_strength_default = 0.1;



      // TODO At least initially, would it be safer to assume that a coefficient is always provided?
      // TODO Later we may want to work more natively with weights;
      //   but that may require structural changes elsewhere
      // TODO Also will want derivatives with respect to weight rather than coefficient
      FORCE_INLINE value_type reg_weight (const value_type coeff)
      {
        return Math::pow2 (std::exp(coeff) - value_type(1.0));
      }
      FORCE_INLINE value_type reg_weight (const value_type coeff, const value_type base)
      {
        return Math::pow2 (std::exp(coeff) - std::exp(base));
      }

      FORCE_INLINE value_type dregweight_dcoeff (const value_type coeff)
      {
        const value_type weight = std::exp (coeff);
        return (value_type(2.0) * weight * (weight - value_type(1.0)));
      }
      FORCE_INLINE value_type dregweight_dcoeff (const value_type coeff, const value_type base)
      {
        const value_type weight = std::exp (coeff);
        return (value_type(2.0) * weight * (weight - std::exp(base)));
      }

      FORCE_INLINE value_type d2regweight_dcoeff2 (const value_type coeff)
      {
        const value_type double_weight = value_type(2.0) * std::exp (coeff);
        return (double_weight * (double_weight - value_type(1.0)));
      }
      FORCE_INLINE value_type d2regweight_dcoeff2 (const value_type coeff, const value_type base)
      {
        const value_type double_weight = value_type(2.0) * std::exp (coeff);
        return (double_weight * (double_weight - std::exp(base)));
      }

      FORCE_INLINE value_type d3regweight_dcoeff3 (const value_type coeff)
      {
        const value_type weight = std::exp (coeff);
        return (value_type(2.0) * weight * ((value_type(4.0) * weight) - value_type(1.0)));
      }
      FORCE_INLINE value_type d3regweight_dcoeff3 (const value_type coeff, const value_type base)
      {
        const value_type weight = std::exp (coeff);
        return (value_type(2.0) * weight * ((value_type(4.0) * weight) - std::exp(base)));
      }




      FORCE_INLINE value_type reg_coeff (const value_type coeff)
      {
        return Math::pow2 (coeff);
      }
      FORCE_INLINE value_type reg_coeff (const value_type coeff, const value_type base)
      {
        return Math::pow2 (coeff - base);
      }

      FORCE_INLINE value_type dregcoeff_dcoeff (const value_type coeff)
      {
        return (value_type(2.0) * coeff);
      }
      FORCE_INLINE value_type dregcoeff_dcoeff (const value_type coeff, const value_type base)
      {
        return (value_type(2.0) * (coeff - base));
      }

      FORCE_INLINE value_type d2regcoeff_dcoeff2 (const value_type coeff)
      {
        return (value_type(2.0));
      }
      FORCE_INLINE value_type d2regcoeff_dcoeff2 (const value_type coeff, const value_type base)
      {
        return (value_type(2.0));
      }

      FORCE_INLINE value_type d3regcoeff_dcoeff3 (const value_type coeff)
      {
        return (value_type(0.0));
      }
      FORCE_INLINE value_type d3regcoeff_dcoeff3 (const value_type coeff, const value_type base)
      {
        return (value_type(0.0));
      }







      // TODO Hypothetically could generate all of these using a macro
      FORCE_INLINE value_type reg_gamma (const value_type coeff)
      {
        return ((coeff <= value_type(0.0)) ? reg_coeff (coeff) : reg_weight (coeff));
      }
      FORCE_INLINE value_type reg_gamma (const value_type coeff, const value_type base)
      {
        return ((coeff <= base) ? reg_coeff (coeff, base) : reg_weight (coeff, base));
      }

      FORCE_INLINE value_type dreggamma_dcoeff (const value_type coeff)
      {
        return ((coeff <= value_type(0.0)) ? dregcoeff_dcoeff (coeff) : dregweight_dcoeff (coeff));
      }
      FORCE_INLINE value_type dreggamma_dcoeff (const value_type coeff, const value_type base)
      {
        return ((coeff <= base) ? dregcoeff_dcoeff (coeff, base) : dregweight_dcoeff (coeff, base));
      }

      FORCE_INLINE value_type d2reggamma_dcoeff2 (const value_type coeff)
      {
        return ((coeff <= value_type(0.0)) ? d2regcoeff_dcoeff2 (coeff) : d2regweight_dcoeff2 (coeff));
      }
      FORCE_INLINE value_type d2reggamma_dcoeff2 (const value_type coeff, const value_type base)
      {
        return ((coeff <= base) ? d2regcoeff_dcoeff2 (coeff, base) : d2regweight_dcoeff2 (coeff, base));
      }

      FORCE_INLINE value_type d3reggamma_dcoeff3 (const value_type coeff)
      {
        return ((coeff <= value_type(0.0)) ? d3regcoeff_dcoeff3 (coeff) : d3regweight_dcoeff3 (coeff));
      }
      FORCE_INLINE value_type d3reggamma_dcoeff3 (const value_type coeff, const value_type base)
      {
        return ((coeff <= base) ? d3regcoeff_dcoeff3 (coeff, base) : d3regweight_dcoeff3 (coeff, base));
      }




      template <reg_fn_t T>
      value_type reg (const value_type coeff);
      template <> FORCE_INLINE value_type reg<reg_fn_t::WEIGHT> (const value_type coeff) { return reg_weight (coeff); }
      template <> FORCE_INLINE value_type reg<reg_fn_t::COEFF>  (const value_type coeff) { return reg_coeff  (coeff); }
      template <> FORCE_INLINE value_type reg<reg_fn_t::GAMMA>  (const value_type coeff) { return reg_gamma  (coeff); }

      template <reg_fn_t T>
      value_type reg (const value_type coeff, const value_type base);
      template <> FORCE_INLINE value_type reg<reg_fn_t::WEIGHT> (const value_type coeff, const value_type base) { return reg_weight (coeff, base); }
      template <> FORCE_INLINE value_type reg<reg_fn_t::COEFF>  (const value_type coeff, const value_type base) { return reg_coeff  (coeff, base); }
      template <> FORCE_INLINE value_type reg<reg_fn_t::GAMMA>  (const value_type coeff, const value_type base) { return reg_gamma  (coeff, base); }




      FORCE_INLINE void dxregweight_dcoeffx (CostAndDerivatives& result, const value_type coeff, const value_type weight, const value_type multiplier)
      {
        const value_type double_weight = value_type(2.0) * weight;
        result.cost         += multiplier * Math::pow2 (weight - value_type(1.0));
        result.first_deriv  += multiplier * double_weight * (weight - value_type(1.0));
        result.second_deriv += multiplier * double_weight * (double_weight - value_type(1.0));
        result.third_deriv  += multiplier * double_weight * ((value_type(4.0) * weight) - value_type(1.0));
      }
      FORCE_INLINE void dxregweight_dcoeffx (CostAndDerivatives& result, const value_type coeff, const value_type weight, const value_type multiplier, const value_type base_coeff, const value_type base_weight)
      {
        result.cost         += multiplier * Math::pow2 (weight - base_weight);
        result.first_deriv  += multiplier * value_type(2.0) * weight * (weight - base_weight);
        result.second_deriv += multiplier * value_type(2.0) * weight * ((value_type(2.0)*weight) - base_weight);
        result.third_deriv  += multiplier * value_type(2.0) * weight * ((value_type(4.0)*weight) - base_weight);
      }

      FORCE_INLINE void dxregcoeff_dcoeffx (CostAndDerivatives& result, const value_type coeff, const value_type weight, const value_type multiplier)
      {
        result.cost         += multiplier * Math::pow2 (coeff);
        result.first_deriv  += multiplier * value_type(2.0) * coeff;
        result.second_deriv += multiplier * value_type(2.0);
      }
      FORCE_INLINE void dxregcoeff_dcoeffx (CostAndDerivatives& result, const value_type coeff, const value_type weight, const value_type multiplier, const value_type base_coeff, const value_type base_weight)
      {
        result.cost         += multiplier * Math::pow2 (coeff - base_coeff);
        result.first_deriv  += multiplier * value_type(2.0) * (coeff - base_coeff);
        result.second_deriv += multiplier * value_type(2.0);
      }

      FORCE_INLINE void dxreggamma_dcoeffx (CostAndDerivatives& result, const value_type coeff, const value_type weight, const value_type multiplier)
      {
        if (coeff <= value_type(0.0))
          dxregcoeff_dcoeffx (result, coeff, weight, multiplier);
        else
          dxregweight_dcoeffx (result, coeff, weight, multiplier);
      }
      FORCE_INLINE void dxreggamma_dcoeffx (CostAndDerivatives& result, const value_type coeff, const value_type weight, const value_type multiplier, const value_type base_coeff, const value_type base_weight)
      {
        if (coeff <= base_coeff)
          dxregcoeff_dcoeffx (result, coeff, weight, multiplier, base_coeff, base_weight);
        else
          dxregweight_dcoeffx (result, coeff, weight, multiplier, base_coeff, base_weight);
      }




      template <reg_fn_t T>
      void dxreg_dcoeffx (CostAndDerivatives& result, const value_type coeff, const value_type weight, const value_type multiplier);
      template <> FORCE_INLINE void dxreg_dcoeffx<reg_fn_t::WEIGHT> (CostAndDerivatives& result, const value_type coeff, const value_type weight, const value_type multiplier) { dxregweight_dcoeffx (result, coeff, weight, multiplier); }
      template <> FORCE_INLINE void dxreg_dcoeffx<reg_fn_t::COEFF>  (CostAndDerivatives& result, const value_type coeff, const value_type weight, const value_type multiplier) { dxregcoeff_dcoeffx  (result, coeff, weight, multiplier); }
      template <> FORCE_INLINE void dxreg_dcoeffx<reg_fn_t::GAMMA>  (CostAndDerivatives& result, const value_type coeff, const value_type weight, const value_type multiplier) { dxreggamma_dcoeffx  (result, coeff, weight, multiplier); }

      template <reg_fn_t T>
      void dxreg_dcoeffx (CostAndDerivatives& result, const value_type coeff, const value_type weight, const value_type multiplier, const value_type base_coeff, const value_type base_weight);
      template <> FORCE_INLINE void dxreg_dcoeffx<reg_fn_t::WEIGHT> (CostAndDerivatives& result, const value_type coeff, const value_type weight, const value_type multiplier, const value_type base_coeff, const value_type base_weight) { dxregweight_dcoeffx (result, coeff, weight, multiplier, base_coeff, base_weight); }
      template <> FORCE_INLINE void dxreg_dcoeffx<reg_fn_t::COEFF>  (CostAndDerivatives& result, const value_type coeff, const value_type weight, const value_type multiplier, const value_type base_coeff, const value_type base_weight) { dxregcoeff_dcoeffx  (result, coeff, weight, multiplier, base_coeff, base_weight); }
      template <> FORCE_INLINE void dxreg_dcoeffx<reg_fn_t::GAMMA>  (CostAndDerivatives& result, const value_type coeff, const value_type weight, const value_type multiplier, const value_type base_coeff, const value_type base_weight) { dxreggamma_dcoeffx  (result, coeff, weight, multiplier, base_coeff, base_weight); }





      }
    }
  }
}


#endif


