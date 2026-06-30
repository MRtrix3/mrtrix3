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
      //   1. Penalises streamline coefficient
      //   2. Penalises streamline weighting factor
      //   3. Utilise reg_gamma function
      //
      // Ideally, for computational performance, these would be templated out,
      //   since they may be involved in derivative calculations in deep line search loops

      enum reg_basis_t { STREAMLINE, FIXEL, GROUP };
      enum reg_fn_abs_t { COEFF, FACTOR, GAMMA };
      enum reg_fn_diff_t { DELTACOEFF, DUALINVBARR };

      extern const char* reg_basis_choices[];
      extern const char* reg_fn_choices_abs[];
      extern const char* reg_fn_choices_diff[];

      extern App::OptionGroup RegularisationOptions;

      constexpr default_type regularisation_strength_abs_default = 0.1;
      // TODO Consider reducing default to 0.01 for differential:
      //   data are bounded and so over-regularising to preclude outliers not so crucial
      constexpr default_type regularisation_strength_diff_default = 0.1;




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

      FORCE_INLINE value_type dregfactor_dcoeff (const WeightingCoeffAndFactor& WCF)
      {
        return (value_type(2) * WCF.factor() * (WCF.factor() - value_type(1.0)));
      }
      FORCE_INLINE value_type dregfactor_dcoeff (const WeightingCoeffAndFactor& WCF, const WeightingCoeffAndFactor& ref)
      {
        return (value_type(2) * WCF.factor() * (WCF.factor() - ref.factor()));
      }

      FORCE_INLINE value_type d2regfactor_dcoeff2 (const WeightingCoeffAndFactor& WCF)
      {
        return (value_type(2) * WCF.factor() * ((value_type(2) * WCF.factor()) - value_type(1.0)));
      }
      FORCE_INLINE value_type d2regfactor_dcoeff2 (const WeightingCoeffAndFactor& WCF, const WeightingCoeffAndFactor& ref)
      {
        return (value_type(2) * WCF.factor() * ((value_type(2) * WCF.factor()) - ref.factor()));
      }

      FORCE_INLINE value_type d3regfactor_dcoeff3 (const WeightingCoeffAndFactor& WCF)
      {
        return (value_type(2) * WCF.factor() * ((value_type(4) * WCF.factor()) - value_type(1.0)));
      }
      FORCE_INLINE value_type d3regfactor_dcoeff3 (const WeightingCoeffAndFactor& WCF, const WeightingCoeffAndFactor& ref)
      {
        return (value_type(2) * WCF.factor() * ((value_type(4) * WCF.factor()) - ref.factor()));
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
        return ((WCF.coeff() <= value_type(0)) ? dregcoeff_dcoeff (WCF) : dregfactor_dcoeff (WCF));
      }
      FORCE_INLINE value_type dreggamma_dcoeff (const WeightingCoeffAndFactor& WCF, const WeightingCoeffAndFactor& ref)
      {
        return ((WCF.coeff() <= ref.coeff()) ? dregcoeff_dcoeff (WCF, ref) : dregfactor_dcoeff (WCF, ref));
      }

      FORCE_INLINE value_type d2reggamma_dcoeff2 (const WeightingCoeffAndFactor& WCF)
      {
        return ((WCF.coeff() <= value_type(0)) ? d2regcoeff_dcoeff2 (WCF) : d2regfactor_dcoeff2 (WCF));
      }
      FORCE_INLINE value_type d2reggamma_dcoeff2 (const WeightingCoeffAndFactor& WCF, const WeightingCoeffAndFactor& ref)
      {
        return ((WCF.coeff() <= ref.coeff()) ? d2regcoeff_dcoeff2 (WCF, ref) : d2regfactor_dcoeff2 (WCF, ref));
      }

      FORCE_INLINE value_type d3reggamma_dcoeff3 (const WeightingCoeffAndFactor& WCF)
      {
        return ((WCF.coeff() <= value_type(0)) ? d3regcoeff_dcoeff3 (WCF) : d3regfactor_dcoeff3 (WCF));
      }
      FORCE_INLINE value_type d3reggamma_dcoeff3 (const WeightingCoeffAndFactor& WCF, const WeightingCoeffAndFactor& ref)
      {
        return ((WCF.coeff() <= ref.coeff()) ? d3regcoeff_dcoeff3 (WCF, ref) : d3regfactor_dcoeff3 (WCF, ref));
      }




      template <reg_fn_abs_t T>
      value_type reg (const WeightingCoeffAndFactor& WCF);
      template <> FORCE_INLINE value_type reg<reg_fn_abs_t::COEFF>  (const WeightingCoeffAndFactor& WCF) { return reg_coeff  (WCF); }
      template <> FORCE_INLINE value_type reg<reg_fn_abs_t::FACTOR> (const WeightingCoeffAndFactor& WCF) { return reg_factor (WCF); }
      template <> FORCE_INLINE value_type reg<reg_fn_abs_t::GAMMA>  (const WeightingCoeffAndFactor& WCF) { return reg_gamma  (WCF); }

      template <reg_fn_abs_t T>
      value_type reg (const WeightingCoeffAndFactor& WCF, const WeightingCoeffAndFactor& ref);
      template <> FORCE_INLINE value_type reg<reg_fn_abs_t::COEFF>  (const WeightingCoeffAndFactor& WCF, const WeightingCoeffAndFactor& ref) { return reg_coeff  (WCF, ref); }
      template <> FORCE_INLINE value_type reg<reg_fn_abs_t::FACTOR> (const WeightingCoeffAndFactor& WCF, const WeightingCoeffAndFactor& ref) { return reg_factor (WCF, ref); }
      template <> FORCE_INLINE value_type reg<reg_fn_abs_t::GAMMA>  (const WeightingCoeffAndFactor& WCF, const WeightingCoeffAndFactor& ref) { return reg_gamma  (WCF, ref); }




      template <reg_fn_abs_t T>
      value_type dreg_dcoeff (const WeightingCoeffAndFactor& WCF);
      template <> FORCE_INLINE value_type dreg_dcoeff<reg_fn_abs_t::COEFF>  (const WeightingCoeffAndFactor& WCF) { return dregcoeff_dcoeff  (WCF); }
      template <> FORCE_INLINE value_type dreg_dcoeff<reg_fn_abs_t::FACTOR> (const WeightingCoeffAndFactor& WCF) { return dregfactor_dcoeff (WCF); }
      template <> FORCE_INLINE value_type dreg_dcoeff<reg_fn_abs_t::GAMMA>  (const WeightingCoeffAndFactor& WCF) { return dreggamma_dcoeff  (WCF); }

      template <reg_fn_abs_t T>
      value_type dreg_dcoeff (const WeightingCoeffAndFactor& WCF, const WeightingCoeffAndFactor& ref);
      template <> FORCE_INLINE value_type dreg_dcoeff<reg_fn_abs_t::COEFF>  (const WeightingCoeffAndFactor& WCF, const WeightingCoeffAndFactor& ref) { return dregcoeff_dcoeff  (WCF, ref); }
      template <> FORCE_INLINE value_type dreg_dcoeff<reg_fn_abs_t::FACTOR> (const WeightingCoeffAndFactor& WCF, const WeightingCoeffAndFactor& ref) { return dregfactor_dcoeff (WCF, ref); }
      template <> FORCE_INLINE value_type dreg_dcoeff<reg_fn_abs_t::GAMMA>  (const WeightingCoeffAndFactor& WCF, const WeightingCoeffAndFactor& ref) { return dreggamma_dcoeff  (WCF, ref); }



      void dxregcoeff_dcoeffx (CostAndDerivatives& result, const WeightingCoeffAndFactor& WCF, const value_type multiplier);
      void dxregcoeff_dcoeffx (CostAndDerivatives& result, const WeightingCoeffAndFactor& WCF, const value_type multiplier, const WeightingCoeffAndFactor& ref);
      void dxregfactor_dcoeffx (CostAndDerivatives& result, const WeightingCoeffAndFactor& WCF, const value_type multiplier);
      void dxregfactor_dcoeffx (CostAndDerivatives& result, const WeightingCoeffAndFactor& WCF, const value_type multiplier, const WeightingCoeffAndFactor& ref);
      void dxreggamma_dcoeffx (CostAndDerivatives& result, const WeightingCoeffAndFactor& WCF, const value_type multiplier);
      void dxreggamma_dcoeffx (CostAndDerivatives& result, const WeightingCoeffAndFactor& WCF, const value_type multiplier, const WeightingCoeffAndFactor& ref);




      template <reg_fn_abs_t T>
      void dxreg_dcoeffx (CostAndDerivatives& result, const WeightingCoeffAndFactor& WCF, const value_type multiplier);
      template <> FORCE_INLINE void dxreg_dcoeffx<reg_fn_abs_t::COEFF>  (CostAndDerivatives& result, const WeightingCoeffAndFactor& WCF, const value_type multiplier) { dxregcoeff_dcoeffx  (result, WCF, multiplier); }
      template <> FORCE_INLINE void dxreg_dcoeffx<reg_fn_abs_t::FACTOR> (CostAndDerivatives& result, const WeightingCoeffAndFactor& WCF, const value_type multiplier) { dxregfactor_dcoeffx (result, WCF, multiplier); }
      template <> FORCE_INLINE void dxreg_dcoeffx<reg_fn_abs_t::GAMMA>  (CostAndDerivatives& result, const WeightingCoeffAndFactor& WCF, const value_type multiplier) { dxreggamma_dcoeffx  (result, WCF, multiplier); }

      template <reg_fn_abs_t T>
      void dxreg_dcoeffx (CostAndDerivatives& result, const WeightingCoeffAndFactor& WCF, const value_type multiplier, const WeightingCoeffAndFactor& ref);
      template <> FORCE_INLINE void dxreg_dcoeffx<reg_fn_abs_t::COEFF>  (CostAndDerivatives& result, const WeightingCoeffAndFactor& WCF, const value_type multiplier, const WeightingCoeffAndFactor& ref) { dxregcoeff_dcoeffx  (result, WCF, multiplier, ref); }
      template <> FORCE_INLINE void dxreg_dcoeffx<reg_fn_abs_t::FACTOR> (CostAndDerivatives& result, const WeightingCoeffAndFactor& WCF, const value_type multiplier, const WeightingCoeffAndFactor& ref) { dxregfactor_dcoeffx (result, WCF, multiplier, ref); }
      template <> FORCE_INLINE void dxreg_dcoeffx<reg_fn_abs_t::GAMMA>  (CostAndDerivatives& result, const WeightingCoeffAndFactor& WCF, const value_type multiplier, const WeightingCoeffAndFactor& ref) { dxreggamma_dcoeffx  (result, WCF, multiplier, ref); }





      // Regularisation functions specific to differential mode
      FORCE_INLINE bool reg_delta_out_of_bounds(const value_type deltacoeff) {
        return (std::abs(deltacoeff) > value_type(1));
      }
      FORCE_INLINE bool reg_delta_out_of_bounds(const DifferentialWCF &dWCF) {
        return reg_delta_out_of_bounds(dWCF.delta_coeff());
      }
      FORCE_INLINE value_type reg_deltacoeff (const value_type deltacoeff) {
        if (reg_delta_out_of_bounds(deltacoeff))
          return std::numeric_limits<value_type>::signaling_NaN();
        return Math::pow2(deltacoeff);
      }
      FORCE_INLINE value_type reg_deltacoeff (const value_type deltacoeff, const value_type ref) {
        assert (!reg_delta_out_of_bounds(ref));
        if (reg_delta_out_of_bounds(deltacoeff))
          return std::numeric_limits<value_type>::signaling_NaN();
        const value_type diff = deltacoeff - ref;
        if (diff == value_type(0))
          return value_type(0);
        return Math::pow2(diff / (deltacoeff < ref         //
                                  ? value_type(1) + ref    //
                                  : value_type(1) - ref)); //
      }
      FORCE_INLINE value_type reg_deltacoeff (const DifferentialWCF &dWCF) {
        return reg_deltacoeff(dWCF.delta_coeff());
      }
      FORCE_INLINE value_type reg_deltacoeff (const DifferentialWCF &dWCF, const value_type ref) {
        return reg_deltacoeff(dWCF.delta_coeff(), ref);
      }
      FORCE_INLINE value_type dregdeltacoeff_ddeltacoeff (const DifferentialWCF &dWCF) {
        if (reg_delta_out_of_bounds(dWCF))
          return std::numeric_limits<value_type>::signaling_NaN();
        return value_type(2) * dWCF.delta_coeff();
      }
      FORCE_INLINE value_type dregdeltacoeff_ddeltacoeff (const DifferentialWCF &dWCF, const value_type ref) {
        assert (!reg_delta_out_of_bounds(ref));
        if (reg_delta_out_of_bounds(dWCF))
          return std::numeric_limits<value_type>::signaling_NaN();
        const value_type diff = dWCF.delta_coeff() - ref;
        if (diff == value_type(0))
          return value_type(0);
        return value_type(2) * diff / Math::pow2(dWCF.delta_coeff() < ref //
                                                 ? value_type(1) + ref    //
                                                 : value_type(1) - ref);  //
      }
      FORCE_INLINE value_type d2regadeltacoeff_ddeltacoeff2 (const DifferentialWCF &dWCF) {
        if (reg_delta_out_of_bounds(dWCF))
          return std::numeric_limits<value_type>::signaling_NaN();
        return value_type(2);
      }
      FORCE_INLINE value_type d2regadeltacoeff_ddeltacoeff2 (const DifferentialWCF &dWCF, const value_type ref) {
        assert (!reg_delta_out_of_bounds(ref));
        if (reg_delta_out_of_bounds(dWCF))
          return std::numeric_limits<value_type>::signaling_NaN();
        if (MR::abs(ref) == value_type(1))
          return value_type(0.5);
        return value_type(2) / Math::pow2(dWCF.delta_coeff() < ref
                                           ? value_type(1) + ref
                                           : value_type(1) - ref);
      }
      FORCE_INLINE value_type d3regdeltacoeff_ddeltacoeff3 (const DifferentialWCF &dWCF) {
        if (reg_delta_out_of_bounds(dWCF))
          return std::numeric_limits<value_type>::signaling_NaN();
        return value_type(0);
      }
      FORCE_INLINE value_type d3regdeltacoeff_ddeltacoeff3 (const DifferentialWCF &dWCF, const value_type ref) {
        if (reg_delta_out_of_bounds(dWCF))
          return std::numeric_limits<value_type>::signaling_NaN();
        return value_type(0);
      }

      namespace {
        value_type transformed_deltacoeff(const value_type deltacoeff, const value_type ref) {
          assert (std::abs(ref) < value_type(1));
          return deltacoeff <= ref
                 ? ((deltacoeff + value_type(1)) / (value_type(1) + ref)) - value_type(1)
                 : ((deltacoeff - value_type(1)) / (value_type(1) - ref)) + value_type(1);
        }
      }
      // FORCE_INLINE bool reg_dualinvbarr_out_of_bounds(const value_type deltacoeff) {
      //   return (std::abs(deltacoeff) >= value_type(1));
      // }
      FORCE_INLINE value_type reg_dualinvbarr (const value_type deltacoeff)
      {
        if (std::abs(deltacoeff) == value_type(1))
          return std::numeric_limits<value_type>::infinity();
        if (std::abs(deltacoeff) > value_type(1))
          return std::numeric_limits<value_type>::signaling_NaN();
        return (value_type(-1) - (value_type(1) /                                                  //
                                  ((deltacoeff - value_type(1)) * (deltacoeff + value_type(1))))); //
      }
      FORCE_INLINE value_type reg_dualinvbarr (const value_type deltacoeff, const value_type ref)
      {
        return reg_dualinvbarr(transformed_deltacoeff(deltacoeff, ref));
      }
      FORCE_INLINE value_type reg_dualinvbarr (const DifferentialWCF &dWCF)
      {
        return reg_dualinvbarr(dWCF.delta_coeff());
      }
      FORCE_INLINE value_type reg_dualinvbarr (const DifferentialWCF &dWCF, const value_type ref)
      {
        return reg_dualinvbarr(dWCF.delta_coeff(), ref);
      }
      FORCE_INLINE value_type dregdualinvbarr_ddeltacoeff (const value_type deltacoeff)
      {
        if (std::abs(deltacoeff) == value_type(1))
          return std::numeric_limits<value_type>::infinity();
        if (std::abs(deltacoeff) > value_type(1))
          return std::numeric_limits<value_type>::signaling_NaN();
        return (value_type(2) * deltacoeff /                                                        //
                (Math::pow2(deltacoeff - value_type(1)) * Math::pow2(deltacoeff + value_type(1)))); //
      }
      FORCE_INLINE value_type dregdualinvbarr_ddeltacoeff (const DifferentialWCF &dWCF)
      {
        return dregdualinvbarr_ddeltacoeff(dWCF.delta_coeff());
      }
      FORCE_INLINE value_type dregdualinvbarr_ddeltacoeff (const value_type deltacoeff, const value_type ref)
      {
        if (std::abs(deltacoeff) == value_type(1))
          return std::numeric_limits<value_type>::infinity();
        if (std::abs(deltacoeff) > value_type(1))
          return std::numeric_limits<value_type>::signaling_NaN();
        const value_type X = transformed_deltacoeff(deltacoeff, ref);
        const value_type dX_ddeltacoeff = value_type(1) / (deltacoeff <= ref       //
                                                           ? value_type(1) + ref   //
                                                           : value_type(1) - ref); //
        return dregdualinvbarr_ddeltacoeff(X) * dX_ddeltacoeff;
      }
      FORCE_INLINE value_type dregdualinvbarr_ddeltacoeff (const DifferentialWCF &dWCF, const value_type ref)
      {
        return dregdualinvbarr_ddeltacoeff (dWCF.delta_coeff(), ref);
      }
      FORCE_INLINE value_type d2regdualinvbarr_ddeltacoeff2 (const value_type deltacoeff)
      {
        if (std::abs(deltacoeff) == value_type(1))
          return std::numeric_limits<value_type>::infinity();
        if (std::abs(deltacoeff) > value_type(1))
          return std::numeric_limits<value_type>::signaling_NaN();
        return (value_type(-2) * ((value_type(3) * Math::pow2(deltacoeff)) + value_type(1)) /       //
                (Math::pow3(deltacoeff - value_type(1)) * Math::pow3(deltacoeff + value_type(1)))); //
      }
      FORCE_INLINE value_type d2regdualinvbarr_ddeltacoeff2 (const DifferentialWCF &dWCF)
      {
        return d2regdualinvbarr_ddeltacoeff2(dWCF.delta_coeff());
      }
      FORCE_INLINE value_type d2regdualinvbarr_ddeltacoeff2 (const value_type deltacoeff, const value_type ref)
      {
        if (std::abs(deltacoeff) == value_type(1))
          return std::numeric_limits<value_type>::infinity();
        if (std::abs(deltacoeff) > value_type(1))
          return std::numeric_limits<value_type>::signaling_NaN();
        const value_type X = transformed_deltacoeff(deltacoeff, ref);
        const value_type dX_ddeltacoeff = value_type(1) / (deltacoeff <= ref       //
                                                           ? value_type(1) + ref   //
                                                           : value_type(1) - ref); //
        return d2regdualinvbarr_ddeltacoeff2(X) * Math::pow2(dX_ddeltacoeff);
      }
      FORCE_INLINE value_type d2regdualinvbarr_ddeltacoeff2 (const DifferentialWCF &dWCF, const value_type ref)
      {
        return d2regdualinvbarr_ddeltacoeff2(dWCF.delta_coeff(), ref);
      }
      FORCE_INLINE value_type d3regdualinvbarr_ddeltacoeff3 (const value_type deltacoeff)
      {
        if (std::abs(deltacoeff) == value_type(1))
          return std::numeric_limits<value_type>::infinity();
        if (std::abs(deltacoeff) > value_type(1))
          return std::numeric_limits<value_type>::signaling_NaN();
        return (value_type(24) * deltacoeff * (Math::pow2(deltacoeff) + value_type(1)) /            //
                (Math::pow4(deltacoeff - value_type(1)) * Math::pow4(deltacoeff + value_type(1)))); //
      }
      FORCE_INLINE value_type d3regdualinvbarr_ddeltacoeff3 (const DifferentialWCF &dWCF)
      {
        return d3regdualinvbarr_ddeltacoeff3(dWCF.delta_coeff());
      }
      FORCE_INLINE value_type d3regdualinvbarr_ddeltacoeff3 (const value_type deltacoeff, const value_type ref)
      {
        if (std::abs(deltacoeff) == value_type(1))
          return std::numeric_limits<value_type>::infinity();
        if (std::abs(deltacoeff) > value_type(1))
          return std::numeric_limits<value_type>::signaling_NaN();
        const value_type X = transformed_deltacoeff(deltacoeff, ref);
        const value_type dX_ddeltacoeff = value_type(1) / (deltacoeff <= ref       //
                                                           ? value_type(1) + ref   //
                                                           : value_type(1) - ref); //
        return d3regdualinvbarr_ddeltacoeff3(X) * Math::pow3(dX_ddeltacoeff);
      }
      FORCE_INLINE value_type d3regdualinvbarr_ddeltacoeff3 (const DifferentialWCF &dWCF, const value_type ref)
      {
        return d3regdualinvbarr_ddeltacoeff3(dWCF.delta_coeff(), ref);
      }



      void dxregdualinvbarr_ddeltacoeffx (CostAndDerivatives& result, const value_type deltacoeff, const value_type multiplier);
      void dxregdualinvbarr_ddeltacoeffx (CostAndDerivatives& result, const DifferentialWCF &dWCF, const value_type multiplier);
      void dxregdualinvbarr_ddeltacoeffx (CostAndDerivatives& result, const value_type deltacoeff, const value_type multiplier, const value_type ref);
      void dxregdualinvbarr_ddeltacoeffx (CostAndDerivatives& result, const DifferentialWCF &dWCF, const value_type multiplier, const value_type ref);
      void dxregdeltacoeff_ddeltacoeffx (CostAndDerivatives& result, const DifferentialWCF &dWCF, const value_type multiplier);
      void dxregdeltacoeff_ddeltacoeffx (CostAndDerivatives& result, const DifferentialWCF &dWCF, const value_type multiplier, const value_type ref);



      template <reg_fn_diff_t T>
      value_type reg (const value_type deltacoeff);
      template <> FORCE_INLINE value_type reg<reg_fn_diff_t::DELTACOEFF>  (const value_type deltacoeff) { return reg_deltacoeff  (deltacoeff); }
      template <> FORCE_INLINE value_type reg<reg_fn_diff_t::DUALINVBARR> (const value_type deltacoeff) { return reg_dualinvbarr (deltacoeff); }

      template <reg_fn_diff_t T>
      value_type reg (const DifferentialWCF &dWCF);
      template <> FORCE_INLINE value_type reg<reg_fn_diff_t::DELTACOEFF>  (const DifferentialWCF &dWCF) { return reg_deltacoeff  (dWCF); }
      template <> FORCE_INLINE value_type reg<reg_fn_diff_t::DUALINVBARR> (const DifferentialWCF &dWCF) { return reg_dualinvbarr (dWCF); }

      template <reg_fn_diff_t T>
      value_type reg (const value_type deltacoeff, const value_type ref);
      template <> FORCE_INLINE value_type reg<reg_fn_diff_t::DELTACOEFF>  (const value_type deltacoeff, const value_type ref) { return reg_deltacoeff  (deltacoeff, ref); }
      template <> FORCE_INLINE value_type reg<reg_fn_diff_t::DUALINVBARR> (const value_type deltacoeff, const value_type ref) { return reg_dualinvbarr (deltacoeff, ref); }

      template <reg_fn_diff_t T>
      value_type reg (const DifferentialWCF &dWCF, const value_type ref);
      template <> FORCE_INLINE value_type reg<reg_fn_diff_t::DELTACOEFF>  (const DifferentialWCF &dWCF, const value_type ref) { return reg_deltacoeff  (dWCF, ref); }
      template <> FORCE_INLINE value_type reg<reg_fn_diff_t::DUALINVBARR> (const DifferentialWCF &dWCF, const value_type ref) { return reg_dualinvbarr (dWCF, ref); }


      template <reg_fn_diff_t T>
      void dxreg_ddeltacoeffx (CostAndDerivatives& result, const DifferentialWCF &dWCF, const value_type multiplier);
      template <> FORCE_INLINE void dxreg_ddeltacoeffx<reg_fn_diff_t::DELTACOEFF>  (CostAndDerivatives& result, const DifferentialWCF &dWCF, const value_type multiplier) { dxregdeltacoeff_ddeltacoeffx  (result, dWCF, multiplier); }
      template <> FORCE_INLINE void dxreg_ddeltacoeffx<reg_fn_diff_t::DUALINVBARR> (CostAndDerivatives& result, const DifferentialWCF &dWCF, const value_type multiplier) { dxregdualinvbarr_ddeltacoeffx (result, dWCF, multiplier); }

      template <reg_fn_diff_t T>
      void dxreg_ddeltacoeffx (CostAndDerivatives& result, const DifferentialWCF &dWCF, const value_type multiplier, const value_type ref);
      template <> FORCE_INLINE void dxreg_ddeltacoeffx<reg_fn_diff_t::DELTACOEFF>  (CostAndDerivatives& result, const DifferentialWCF &dWCF, const value_type multiplier, const value_type ref) { dxregdeltacoeff_ddeltacoeffx  (result, dWCF, multiplier, ref); }
      template <> FORCE_INLINE void dxreg_ddeltacoeffx<reg_fn_diff_t::DUALINVBARR> (CostAndDerivatives& result, const DifferentialWCF &dWCF, const value_type multiplier, const value_type ref) { dxregdualinvbarr_ddeltacoeffx (result, dWCF, multiplier, ref); }










      }
    }
  }
}


#endif


