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
#include "dwi/tractography/SIFT2/types.h"


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



      // New type definitions for regularisation
      // Instead of "tik" and "aTV" regularisation,
      //   will instead have a singular form of regularisation applied in any invocation;
      //   that regularisation will be defined by:
      // - Is it applied on a per-streamline or a per-fixel basis?
      //   (with the latter being with respect to the fixel-wise mean coefficient)
      // - What functional form does the regularisation function take?
      //   1. Penalises streamline weighting factor
      //   2. Penalises streamline coefficient
      //   3. Utilise reg_gamma function
      //
      // Ideally, for computational performance, these would be templated out,
      //   since they may be involved in derivative calculations in deep line search loops

      // TODO Experiment with non-strong enums,
      //   since we want to be able to template by these
      enum reg_basis_t { STREAMLINE, FIXEL };
      enum reg_fn_t { FACTOR, COEFF, GAMMA };

      extern const char* reg_basis_choices[];
      extern const char* reg_fn_choices[];

      extern App::OptionGroup RegularisationOptions;

      constexpr default_type regularisation_strength_abs_default = 0.1;
      constexpr default_type regularisation_strength_diff_default = 0.1;



      // TODO At least initially, would it be safer to assume that a coefficient is always provided?
      // TODO Later we may want to work more natively with weights;
      //   but that may require structural changes elsewhere
      // TODO Also will want derivatives with respect to weighting factor rather than coefficient
      FORCE_INLINE value_type reg_factor (const value_type factor)
      {
        return Math::pow2 (factor - value_type(1.0));
      }
      FORCE_INLINE value_type reg_factor (const WeightingCoeffAndFactor& WCF)
      {
        return reg_factor (WCF.factor());
      }
      FORCE_INLINE value_type reg_factor (const value_type factor, const value_type ref)
      {
        return Math::pow2 (factor - ref);
      }
      FORCE_INLINE value_type reg_factor (const WeightingCoeffAndFactor& WCF, const WeightingCoeffAndFactor& ref)
      {
        return reg_factor (WCF.factor(), ref.factor());
      }

      FORCE_INLINE value_type dregweight_dcoeff (const WeightingCoeffAndFactor& WCF)
      {
        return (value_type(2) * WCF.factor() * (WCF.factor() - value_type(1.0)));
      }
      FORCE_INLINE value_type dregweight_dcoeff (const WeightingCoeffAndFactor& WCF, const WeightingCoeffAndFactor& ref)
      {
        return (value_type(2) * WCF.factor() * (WCF.factor() - ref.factor()));
      }

      FORCE_INLINE value_type d2regweight_dcoeff2 (const WeightingCoeffAndFactor& WCF)
      {
        return (value_type(2) * WCF.factor() * ((value_type(2) * WCF.factor()) - value_type(1.0)));
      }
      FORCE_INLINE value_type d2regweight_dcoeff2 (const WeightingCoeffAndFactor& WCF, const WeightingCoeffAndFactor& ref)
      {
        return (value_type(2) * WCF.factor() * ((value_type(2) * WCF.factor()) - ref.factor()));
      }

      FORCE_INLINE value_type d3regweight_dcoeff3 (const WeightingCoeffAndFactor& WCF)
      {
        return (value_type(2) * WCF.factor() * ((value_type(4) * WCF.factor()) - value_type(1.0)));
      }
      FORCE_INLINE value_type d3regweight_dcoeff3 (const WeightingCoeffAndFactor& WCF, const WeightingCoeffAndFactor& ref)
      {
        return (value_type(2) * WCF.factor() * ((value_type(4) * WCF.factor()) - ref.factor()));
      }




      FORCE_INLINE value_type reg_coeff (const value_type coeff)
      {
        return Math::pow2 (coeff);
      }
      FORCE_INLINE value_type reg_coeff (const WeightingCoeffAndFactor& WCF)
      {
        return reg_coeff (WCF.coeff());
      }
      FORCE_INLINE value_type reg_coeff (const value_type coeff, const value_type ref)
      {
        return Math::pow2 (coeff - ref);
      }
      FORCE_INLINE value_type reg_coeff (const WeightingCoeffAndFactor& WCF, const WeightingCoeffAndFactor& ref)
      {
        return reg_coeff (WCF.coeff(), ref.coeff());
      }

      FORCE_INLINE value_type dregcoeff_dcoeff (const WeightingCoeffAndFactor& WCF)
      {
        return (value_type(2) * WCF.coeff());
      }
      FORCE_INLINE value_type dregcoeff_dcoeff (const WeightingCoeffAndFactor& WCF, const WeightingCoeffAndFactor& ref)
      {
        return (value_type(2) * (WCF.coeff() - ref.coeff()));
      }

      FORCE_INLINE value_type d2regcoeff_dcoeff2 (const WeightingCoeffAndFactor& WCF)
      {
        return (value_type(2));
      }
      FORCE_INLINE value_type d2regcoeff_dcoeff2 (const WeightingCoeffAndFactor& WCF, const WeightingCoeffAndFactor& ref)
      {
        return (value_type(2));
      }

      FORCE_INLINE value_type d3regcoeff_dcoeff3 (const WeightingCoeffAndFactor& WCF)
      {
        return (value_type(0));
      }
      FORCE_INLINE value_type d3regcoeff_dcoeff3 (const WeightingCoeffAndFactor& WCF, const WeightingCoeffAndFactor& ref)
      {
        return (value_type(0));
      }







      // TODO Hypothetically could generate all of these using a macro
      FORCE_INLINE value_type reg_gamma (const WeightingCoeffAndFactor& WCF)
      {
        return ((WCF.coeff() <= value_type(0)) ? reg_coeff (WCF) : reg_factor (WCF));
      }
      FORCE_INLINE value_type reg_gamma (const WeightingCoeffAndFactor& WCF, const WeightingCoeffAndFactor& ref)
      {
        return ((WCF.coeff() <= ref.coeff()) ? reg_coeff (WCF, ref) : reg_factor (WCF, ref));
      }

      FORCE_INLINE value_type dreggamma_dcoeff (const WeightingCoeffAndFactor& WCF)
      {
        return ((WCF.coeff() <= value_type(0)) ? dregcoeff_dcoeff (WCF) : dregweight_dcoeff (WCF));
      }
      FORCE_INLINE value_type dreggamma_dcoeff (const WeightingCoeffAndFactor& WCF, const WeightingCoeffAndFactor& ref)
      {
        return ((WCF.coeff() <= ref.coeff()) ? dregcoeff_dcoeff (WCF, ref) : dregweight_dcoeff (WCF, ref));
      }

      FORCE_INLINE value_type d2reggamma_dcoeff2 (const WeightingCoeffAndFactor& WCF)
      {
        return ((WCF.coeff() <= value_type(0)) ? d2regcoeff_dcoeff2 (WCF) : d2regweight_dcoeff2 (WCF));
      }
      FORCE_INLINE value_type d2reggamma_dcoeff2 (const WeightingCoeffAndFactor& WCF, const WeightingCoeffAndFactor& ref)
      {
        return ((WCF.coeff() <= ref.coeff()) ? d2regcoeff_dcoeff2 (WCF, ref) : d2regweight_dcoeff2 (WCF, ref));
      }

      FORCE_INLINE value_type d3reggamma_dcoeff3 (const WeightingCoeffAndFactor& WCF)
      {
        return ((WCF.coeff() <= value_type(0)) ? d3regcoeff_dcoeff3 (WCF) : d3regweight_dcoeff3 (WCF));
      }
      FORCE_INLINE value_type d3reggamma_dcoeff3 (const WeightingCoeffAndFactor& WCF, const WeightingCoeffAndFactor& ref)
      {
        return ((WCF.coeff() <= ref.coeff()) ? d3regcoeff_dcoeff3 (WCF, ref) : d3regweight_dcoeff3 (WCF, ref));
      }




      template <reg_fn_t T>
      value_type reg (const WeightingCoeffAndFactor& WCF);
      template <> FORCE_INLINE value_type reg<reg_fn_t::FACTOR> (const WeightingCoeffAndFactor& WCF) { return reg_factor (WCF); }
      template <> FORCE_INLINE value_type reg<reg_fn_t::COEFF>  (const WeightingCoeffAndFactor& WCF) { return reg_coeff  (WCF); }
      template <> FORCE_INLINE value_type reg<reg_fn_t::GAMMA>  (const WeightingCoeffAndFactor& WCF) { return reg_gamma  (WCF); }

      template <reg_fn_t T>
      value_type reg (const WeightingCoeffAndFactor& WCF, const WeightingCoeffAndFactor& ref);
      template <> FORCE_INLINE value_type reg<reg_fn_t::FACTOR> (const WeightingCoeffAndFactor& WCF, const WeightingCoeffAndFactor& ref) { return reg_factor (WCF, ref); }
      template <> FORCE_INLINE value_type reg<reg_fn_t::COEFF>  (const WeightingCoeffAndFactor& WCF, const WeightingCoeffAndFactor& ref) { return reg_coeff  (WCF, ref); }
      template <> FORCE_INLINE value_type reg<reg_fn_t::GAMMA>  (const WeightingCoeffAndFactor& WCF, const WeightingCoeffAndFactor& ref) { return reg_gamma  (WCF, ref); }




      template <reg_fn_t T>
      value_type dreg_dcoeff (const WeightingCoeffAndFactor& WCF);
      template <> FORCE_INLINE value_type dreg_dcoeff<reg_fn_t::FACTOR> (const WeightingCoeffAndFactor& WCF) { return dregweight_dcoeff (WCF); }
      template <> FORCE_INLINE value_type dreg_dcoeff<reg_fn_t::COEFF>  (const WeightingCoeffAndFactor& WCF) { return dregcoeff_dcoeff  (WCF); }
      template <> FORCE_INLINE value_type dreg_dcoeff<reg_fn_t::GAMMA>  (const WeightingCoeffAndFactor& WCF) { return dreggamma_dcoeff  (WCF); }

      template <reg_fn_t T>
      value_type dreg_dcoeff (const WeightingCoeffAndFactor& WCF, const WeightingCoeffAndFactor& ref);
      template <> FORCE_INLINE value_type dreg_dcoeff<reg_fn_t::FACTOR> (const WeightingCoeffAndFactor& WCF, const WeightingCoeffAndFactor& ref) { return dregweight_dcoeff (WCF, ref); }
      template <> FORCE_INLINE value_type dreg_dcoeff<reg_fn_t::COEFF>  (const WeightingCoeffAndFactor& WCF, const WeightingCoeffAndFactor& ref) { return dregcoeff_dcoeff  (WCF, ref); }
      template <> FORCE_INLINE value_type dreg_dcoeff<reg_fn_t::GAMMA>  (const WeightingCoeffAndFactor& WCF, const WeightingCoeffAndFactor& ref) { return dreggamma_dcoeff  (WCF, ref); }



      // TODO Reorder reference coeff / weighting factor / delta
      FORCE_INLINE void dxregweight_dcoeffx (CostAndDerivatives& result, const WeightingCoeffAndFactor& WCF, const value_type multiplier)
      {
        const value_type double_weight = value_type(2) * WCF.factor();
        result.cost         += multiplier * Math::pow2 (WCF.factor() - value_type(1.0));
        result.first_deriv  += multiplier * double_weight * (WCF.factor() - value_type(1.0));
        result.second_deriv += multiplier * double_weight * (double_weight - value_type(1.0));
        result.third_deriv  += multiplier * double_weight * ((value_type(4) * WCF.factor()) - value_type(1.0));
      }
      FORCE_INLINE void dxregweight_dcoeffx (CostAndDerivatives& result, const WeightingCoeffAndFactor& WCF, const value_type multiplier, const WeightingCoeffAndFactor& ref)
      {
        result.cost         += multiplier * Math::pow2 (WCF.factor() - ref.factor());
        result.first_deriv  += multiplier * value_type(2) * WCF.factor() * (WCF.factor() - ref.factor());
        result.second_deriv += multiplier * value_type(2) * WCF.factor() * ((value_type(2)*WCF.factor()) - ref.factor());
        result.third_deriv  += multiplier * value_type(2) * WCF.factor() * ((value_type(4)*WCF.factor()) - ref.factor());
      }

      FORCE_INLINE void dxregcoeff_dcoeffx (CostAndDerivatives& result, const WeightingCoeffAndFactor& WCF, const value_type multiplier)
      {
        result.cost         += multiplier * Math::pow2 (WCF.coeff());
        result.first_deriv  += multiplier * value_type(2.0) * WCF.coeff();
        result.second_deriv += multiplier * value_type(2.0);
      }
      FORCE_INLINE void dxregcoeff_dcoeffx (CostAndDerivatives& result, const WeightingCoeffAndFactor& WCF, const value_type multiplier, const WeightingCoeffAndFactor& ref)
      {
        result.cost         += multiplier * Math::pow2 (WCF.coeff() - ref.coeff());
        result.first_deriv  += multiplier * value_type(2) * (WCF.coeff() - ref.coeff());
        result.second_deriv += multiplier * value_type(2);
      }

      FORCE_INLINE void dxreggamma_dcoeffx (CostAndDerivatives& result, const WeightingCoeffAndFactor& WCF, const value_type multiplier)
      {
        if (WCF.coeff() <= value_type(0.0))
          dxregcoeff_dcoeffx (result, WCF, multiplier);
        else
          dxregweight_dcoeffx (result, WCF, multiplier);
      }
      FORCE_INLINE void dxreggamma_dcoeffx (CostAndDerivatives& result, const WeightingCoeffAndFactor& WCF, const value_type multiplier, const WeightingCoeffAndFactor& ref)
      {
        if (WCF.coeff() <= ref.coeff())
          dxregcoeff_dcoeffx (result, WCF, multiplier, ref);
        else
          dxregweight_dcoeffx (result, WCF, multiplier, ref);
      }




      template <reg_fn_t T>
      void dxreg_dcoeffx (CostAndDerivatives& result, const WeightingCoeffAndFactor& WCF, const value_type multiplier);
      template <> FORCE_INLINE void dxreg_dcoeffx<reg_fn_t::FACTOR> (CostAndDerivatives& result, const WeightingCoeffAndFactor& WCF, const value_type multiplier) { dxregweight_dcoeffx (result, WCF, multiplier); }
      template <> FORCE_INLINE void dxreg_dcoeffx<reg_fn_t::COEFF>  (CostAndDerivatives& result, const WeightingCoeffAndFactor& WCF, const value_type multiplier) { dxregcoeff_dcoeffx  (result, WCF, multiplier); }
      template <> FORCE_INLINE void dxreg_dcoeffx<reg_fn_t::GAMMA>  (CostAndDerivatives& result, const WeightingCoeffAndFactor& WCF, const value_type multiplier) { dxreggamma_dcoeffx  (result, WCF, multiplier); }

      template <reg_fn_t T>
      void dxreg_dcoeffx (CostAndDerivatives& result, const WeightingCoeffAndFactor& WCF, const value_type multiplier, const WeightingCoeffAndFactor& ref);
      template <> FORCE_INLINE void dxreg_dcoeffx<reg_fn_t::FACTOR> (CostAndDerivatives& result, const WeightingCoeffAndFactor& WCF, const value_type multiplier, const WeightingCoeffAndFactor& ref) { dxregweight_dcoeffx (result, WCF, multiplier, ref); }
      template <> FORCE_INLINE void dxreg_dcoeffx<reg_fn_t::COEFF>  (CostAndDerivatives& result, const WeightingCoeffAndFactor& WCF, const value_type multiplier, const WeightingCoeffAndFactor& ref) { dxregcoeff_dcoeffx  (result, WCF, multiplier, ref); }
      template <> FORCE_INLINE void dxreg_dcoeffx<reg_fn_t::GAMMA>  (CostAndDerivatives& result, const WeightingCoeffAndFactor& WCF, const value_type multiplier, const WeightingCoeffAndFactor& ref) { dxreggamma_dcoeffx  (result, WCF, multiplier, ref); }





      // Regularisation for differential mode

      FORCE_INLINE value_type reg_delta (const value_type delta)
      {
        return Math::pow2 (delta);
      }
      FORCE_INLINE value_type reg_delta (const value_type delta, const value_type ref)
      {
        return Math::pow2 (delta - ref);
      }

      FORCE_INLINE value_type dregdelta_ddelta (const value_type delta)
      {
        return (value_type(2) * delta);
      }
      FORCE_INLINE value_type dregdelta_ddelta (const value_type delta, const value_type ref)
      {
        return (value_type(2) * (delta - ref));
      }

      FORCE_INLINE value_type d2regdelta_ddelta2 (const value_type delta)
      {
        return (value_type(2));
      }
      FORCE_INLINE value_type d2regdelta_ddelta2 (const value_type delta, const value_type ref)
      {
        return (value_type(2));
      }

      FORCE_INLINE value_type d3regdelta_ddelta3 (const value_type delta)
      {
        return (value_type(0));
      }
      FORCE_INLINE value_type d3regdelta_ddelta3 (const value_type delta, const value_type ref)
      {
        return (value_type(0));
      }




      FORCE_INLINE void dxregdelta_ddeltax (CostAndDerivatives& result, const value_type delta, const value_type multiplier)
      {
        result.cost         += multiplier * Math::pow2 (delta);
        result.first_deriv  += multiplier * value_type(2) * delta;
        result.second_deriv += multiplier * value_type(2);
      }
      FORCE_INLINE void dxregdelta_ddeltax (CostAndDerivatives& result, const value_type delta, const value_type multiplier, const value_type ref)
      {
        result.cost         += multiplier * Math::pow2 (delta - ref);
        result.first_deriv  += multiplier * value_type(2) * (delta - ref);
        result.second_deriv += multiplier * value_type(2);
      }











      }
    }
  }
}


#endif


