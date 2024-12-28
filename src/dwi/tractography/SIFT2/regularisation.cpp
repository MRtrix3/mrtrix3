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



#include "dwi/tractography/SIFT2/regularisation.h"


namespace MR
{
  namespace DWI
  {
    namespace Tractography
    {
      namespace SIFT2
      {



      using namespace App;



      const char* reg_basis_choices[] = { "streamline", "fixel", nullptr };
      const char* reg_fn_choices_abs[] = { "coefficient", "factor", "gamma", nullptr };
      const char* reg_fn_choices_diff[] = { "delta", "dualinvbarr", nullptr };


      OptionGroup RegularisationOptions = OptionGroup ("Options relating to SIFT2 algorithm regularisation")

      // TODO Transfer a primary explanation of the different regularisation bases to a Description paragraph
      + Option ("reg_basis_abs", "The basis upon which regularisation is applied when optimising absolute weights; "
                                 "options are: "
                                 "streamline (regularisation is applied independently to each streamline), "
                                 "fixel (regularisation drives those streamlines traversing each fixel toward a common value) (default)")
        + Argument ("choice").type_choice(reg_basis_choices)

      + Option ("reg_fn_abs", "The form of the regularisation function when optimising absolute weights; "
                              "options are: "
                              "coefficient (quadratically penalise difference of exponential coefficients to zero / fixel mean); "
                              "factor (quadratically penalise difference of streamline weights to one / fixel mean); "
                              "gamma (penalises coefficient if down-reulating, factor if up-regulating) (default)")
        + Argument ("choice").type_choice(reg_fn_choices_abs)

      + Option ("reg_strength_abs", "modulate strength of applied regularisation when optimising absolute weights "
                                    "(default: " + str(regularisation_strength_abs_default, 2) + ")")
        + Argument ("value").type_float (0.0)

      + Option ("reg_basis_diff", "The basis upon which regularisation is applied when optimising differential weights; "
                                  "options are: "
                                  "streamline (regularisation is applied independently to each streamline) (default), "
                                  "fixel (regularisation drives those streamlines traversing each fixel toward a common value)")
        + Argument ("choice").type_choice(reg_basis_choices)

      + Option ("reg_fn_diff", "The form of the regularisation function when optimising differential weights; "
                               "options are: "
                               "delta (quadratically penalise delta coefficient); "
                               "dualinvbarr (\"dual inverse barrier\"; "
                                   "regularisation expression specifically tailored to differential mode) (default)")
        + Argument ("choice").type_choice(reg_fn_choices_diff)

      + Option ("reg_strength_diff", "modulate strength of applied regularisation when optimising differential weights "
                                     "(default: " + str(regularisation_strength_diff_default, 2) + ")")
        + Argument ("value").type_float (0.0);






      void dxregcoeff_dcoeffx (CostAndDerivatives& result, const WeightingCoeffAndFactor& WCF, const value_type multiplier)
      {
        result.cost         += multiplier * Math::pow2 (WCF.coeff());
        result.first_deriv  += multiplier * value_type(2.0) * WCF.coeff();
        result.second_deriv += multiplier * value_type(2.0);
      }
      void dxregcoeff_dcoeffx (CostAndDerivatives& result, const WeightingCoeffAndFactor& WCF, const value_type multiplier, const WeightingCoeffAndFactor& ref)
      {
        result.cost         += multiplier * Math::pow2 (WCF.coeff() - ref.coeff());
        result.first_deriv  += multiplier * value_type(2) * (WCF.coeff() - ref.coeff());
        result.second_deriv += multiplier * value_type(2);
      }

      void dxregfactor_dcoeffx (CostAndDerivatives& result, const WeightingCoeffAndFactor& WCF, const value_type multiplier)
      {
        const value_type double_factor = value_type(2) * WCF.factor();
        result.cost         += multiplier * Math::pow2 (WCF.factor() - value_type(1.0));
        result.first_deriv  += multiplier * double_factor * (WCF.factor() - value_type(1.0));
        result.second_deriv += multiplier * double_factor * (double_factor - value_type(1.0));
        result.third_deriv  += multiplier * double_factor * ((value_type(4) * WCF.factor()) - value_type(1.0));
      }
      void dxregfactor_dcoeffx (CostAndDerivatives& result, const WeightingCoeffAndFactor& WCF, const value_type multiplier, const WeightingCoeffAndFactor& ref)
      {
        result.cost         += multiplier * Math::pow2 (WCF.factor() - ref.factor());
        result.first_deriv  += multiplier * value_type(2) * WCF.factor() * (WCF.factor() - ref.factor());
        result.second_deriv += multiplier * value_type(2) * WCF.factor() * ((value_type(2)*WCF.factor()) - ref.factor());
        result.third_deriv  += multiplier * value_type(2) * WCF.factor() * ((value_type(4)*WCF.factor()) - ref.factor());
      }

      void dxreggamma_dcoeffx (CostAndDerivatives& result, const WeightingCoeffAndFactor& WCF, const value_type multiplier)
      {
        if (WCF.coeff() <= value_type(0.0))
          dxregcoeff_dcoeffx (result, WCF, multiplier);
        else
          dxregfactor_dcoeffx (result, WCF, multiplier);
      }
      void dxreggamma_dcoeffx (CostAndDerivatives& result, const WeightingCoeffAndFactor& WCF, const value_type multiplier, const WeightingCoeffAndFactor& ref)
      {
        if (WCF.coeff() <= ref.coeff())
          dxregcoeff_dcoeffx (result, WCF, multiplier, ref);
        else
          dxregfactor_dcoeffx (result, WCF, multiplier, ref);
      }






      void dxregdualinvbarr_ddeltacoeffx (CostAndDerivatives& result, const value_type deltacoeff, const value_type multiplier)
      {
        if (multiplier == value_type(0))
          return;
        if (reg_dualinvbarr_out_of_bounds(deltacoeff)) {
          result.cost = std::numeric_limits<value_type>::infinity();
          result.first_deriv = std::numeric_limits<value_type>::quiet_NaN();
          result.second_deriv = std::numeric_limits<value_type>::quiet_NaN();
          result.third_deriv = std::numeric_limits<value_type>::quiet_NaN();
          return;
        }
        const value_type dminus1 = deltacoeff - value_type(1);
        const value_type dplus1 = deltacoeff + value_type(1);
        const value_type dminus1_pow2 = Math::pow2(dminus1);
        const value_type dplus1_pow2 = Math::pow2(dplus1);
        const value_type dminus1_pow3 = dminus1_pow2 * dminus1;
        const value_type dplus1_pow3 = dplus1_pow2 * dplus1;
        const value_type dminus1_pow4 = dminus1_pow3 * dminus1;
        const value_type dplus1_pow4 = dplus1_pow3 * dplus1;
        result.cost         += multiplier * (value_type(-1) - (value_type(1) / //
                                             (dminus1 * dplus1)));             //
        result.first_deriv  += multiplier * (value_type(2) * deltacoeff /   //
                                             (dminus1_pow2 * dplus1_pow2)); //
        result.second_deriv += multiplier * (value_type(-2) * ((value_type(3) * Math::pow2(deltacoeff)) + value_type(1)) / //
                                             (dminus1_pow3 * dplus1_pow3));                                                //
        result.third_deriv  += multiplier * (value_type(24) * deltacoeff * (Math::pow2(deltacoeff) + value_type(1)) / //
                                             (dminus1_pow4 * dplus1_pow4));                                           //
      }
      void dxregdualinvbarr_ddeltacoeffx (CostAndDerivatives& result, const DifferentialWCF &dWCF, const value_type multiplier)
      {
        dxregdualinvbarr_ddeltacoeffx (result, dWCF.delta_coeff(), multiplier);
      }

      void dxregdualinvbarr_ddeltacoeffx (CostAndDerivatives& result, const value_type deltacoeff, const value_type multiplier, const value_type ref)
      {
        if (multiplier == value_type(0))
          return;
        if (reg_dualinvbarr_out_of_bounds(deltacoeff)) {
          result.cost = std::numeric_limits<value_type>::infinity();
          result.first_deriv = std::numeric_limits<value_type>::quiet_NaN();
          result.second_deriv = std::numeric_limits<value_type>::quiet_NaN();
          result.third_deriv = std::numeric_limits<value_type>::quiet_NaN();
        }
        const value_type X = transformed_deltacoeff(deltacoeff, ref);
        const value_type dX_ddeltacoeff = value_type(1) / (deltacoeff <= ref       //
                                                           ? value_type(1) + ref   //
                                                           : value_type(1) - ref); //
        const value_type d2X_ddeltacoeff2 = Math::pow2(dX_ddeltacoeff);
        const value_type d3X_ddeltacoeff3 = dX_ddeltacoeff * d2X_ddeltacoeff2;
        CostAndDerivatives temp;
        dxregdualinvbarr_ddeltacoeffx(temp, X, value_type(1));
        result.cost         += multiplier * temp.cost;
        result.first_deriv  += multiplier * temp.first_deriv * dX_ddeltacoeff;
        result.second_deriv += multiplier * temp.second_deriv * d2X_ddeltacoeff2;
        result.third_deriv  += multiplier * temp.third_deriv * d3X_ddeltacoeff3;
      }

      void dxregdualinvbarr_ddeltacoeffx (CostAndDerivatives& result, const DifferentialWCF &dWCF, const value_type multiplier, const value_type ref)
      {
        dxregdualinvbarr_ddeltacoeffx(result, dWCF.delta_coeff(), multiplier, ref);
      }

      void dxregdeltacoeff_ddeltacoeffx (CostAndDerivatives& result, const DifferentialWCF &dWCF, const value_type multiplier)
      {
        result.cost += multiplier * Math::pow2(dWCF.delta_coeff());
        result.first_deriv += multiplier * value_type(2) * dWCF.delta_coeff();
        result.second_deriv += multiplier * value_type(2);
      }

      void dxregdeltacoeff_ddeltacoeffx (CostAndDerivatives& result, const DifferentialWCF &dWCF, const value_type multiplier, const value_type ref)
      {
        const value_type diff = dWCF.delta_coeff() - ref;
        const value_type one_over_denominator = value_type(1) / (diff <= value_type(0)
                                                                 ? Math::pow2(value_type(1) + ref)
                                                                 : Math::pow2(value_type(1) - ref));
        result.cost += multiplier * Math::pow2(diff) * one_over_denominator;
        result.first_deriv += multiplier * value_type(2) * diff * one_over_denominator;
        result.second_deriv += multiplier * value_type(2) * one_over_denominator;
      }




      }
    }
  }
}
