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
#include "dwi/tractography/SIFT2/cost_and_derivatives.h"
#include "dwi/tractography/SIFT2/tckfactor.h"
#include "dwi/tractography/SIFT2/types.h"




namespace MR {
  namespace DWI {
    namespace Tractography {
      namespace SIFT2 {



      class TckFactor;



      // Base class containing all features common to cost function & derivatives calculations
      //   for either absolute or differential modes
      class LineSearchFunctorBase
      {
        public:
          using value_type = SIFT::value_type;
          LineSearchFunctorBase (const SIFT::track_t, TckFactor&);

        protected:
          const SIFT::track_t track_index;
          const value_type mu;
          value_type reg_multiplier_streamline;
          value_type reg_multiplier_fixel;


          class FixelBase
          {
            public:
              FixelBase (const SIFT::Track_fixel_contribution&, const TckFactor::Fixel);
              MR::Fixel::index_type index;
              value_type length, weight, cost_frac, SL_eff;
          };

      };



      // TODO Consider templating maximal required derivative order
      class LineSearchFunctorAbsolute : public LineSearchFunctorBase
      {
        public:
          LineSearchFunctorAbsolute (const SIFT::track_t, TckFactor&);

          // TODO Can these be virtualised?
          template <reg_basis_t RegBasis, reg_fn_t RegFn>
          CostAndDerivatives get (const value_type) const;
          template <reg_basis_t RegBasis, reg_fn_t RegFn>
          value_type operator() (const value_type) const;

        protected:

          // Necessary information for each of those fixels traversed by this streamline
          class Fixel : public LineSearchFunctorBase::FixelBase
          {
            public:
            using BaseType = LineSearchFunctorBase::FixelBase;
            Fixel (const SIFT::Track_fixel_contribution&, const TckFactor::Fixel, const WeightingCoeffAndFactor&, const WeightingCoeffAndFactor&);
            value_type TD, dTD_dFs, FD;
            WeightingCoeffAndFactor WCF;
          };

          const value_type Fs;
          vector<Fixel> fixels;

      };



      // TODO Move to .cpp
      template <reg_basis_t RegBasis, reg_fn_t RegFn>
      CostAndDerivatives
      LineSearchFunctorAbsolute::get (const value_type dFs) const
      {
        const WeightingCoeffAndFactor WCF (WeightingCoeffAndFactor::from_coeff (Fs + dFs));

        CostAndDerivatives data_result, reg_result;

        for (vector<Fixel>::const_iterator i = fixels.begin(); i != fixels.end(); ++i) {

          const value_type contribution = i->length * WCF.factor();
          const value_type scaled_contribution = mu * contribution;
          const value_type roc_contribution = mu * (contribution + i->dTD_dFs);
          const value_type diff = (mu * (i->TD + contribution + (i->dTD_dFs * dFs))) - i->FD;

          data_result.cost += i->cost_frac * i->weight * Math::pow2 (diff);
          data_result.first_deriv += 2.0 * i->weight * i->cost_frac * (roc_contribution * diff);
          data_result.second_deriv += 2.0 * i->weight * i->cost_frac * (Math::pow2 (roc_contribution) + (scaled_contribution * diff));
          data_result.third_deriv += 2.0 * i->weight * i->cost_frac * scaled_contribution * ((3.0*roc_contribution) + diff);

          if (RegBasis == reg_basis_t::FIXEL)
            SIFT2::dxreg_dcoeffx<RegFn> (reg_result, WCF, i->SL_eff, i->WCF);

        }

        if (RegBasis == reg_basis_t::STREAMLINE)
          SIFT2::dxreg_dcoeffx<RegFn> (reg_result, WCF, reg_multiplier_streamline);
        else
          reg_result *= reg_multiplier_fixel;

        return CostAndDerivatives(data_result, reg_result);
      }




      template <reg_basis_t RegBasis, reg_fn_t RegFn>
      LineSearchFunctorAbsolute::value_type LineSearchFunctorAbsolute::operator() (const value_type dFs) const
      {
        const WeightingCoeffAndFactor WCF (WeightingCoeffAndFactor::from_coeff (Fs+dFs));
        value_type cf_data = value_type(0.0);
        value_type cf_reg (RegBasis == reg_basis_t::STREAMLINE ?
                           reg_multiplier_streamline * SIFT2::reg<RegFn> (WCF) :
                           value_type(0.0));
        for (vector<Fixel>::const_iterator i = fixels.begin(); i != fixels.end(); ++i) {
          cf_data += i->cost_frac * i->weight * Math::pow2 ((mu * (i->TD + (i->length * WCF.factor()) + (i->dTD_dFs*dFs))) - i->FD);
          if (RegBasis == reg_basis_t::FIXEL)
            cf_reg += i->SL_eff * SIFT2::reg<RegFn> (WCF, i->WCF);
        }
        if (RegBasis == reg_basis_t::FIXEL)
          cf_reg *= reg_multiplier_fixel;
        return (cf_data + cf_reg);
      }












      // TODO Believe this is currently not taking into account absolute streamline weight
      class LineSearchFunctorDifferential : public LineSearchFunctorBase
      {
        public:
          LineSearchFunctorDifferential (const SIFT::track_t, TckFactor&);

          // Note: No RegFn here; regularisation can _only_ be applied by delta
          template <reg_basis_t RegBasis>
          CostAndDerivatives get (const value_type) const;
          template <reg_basis_t RegBasis>
          value_type operator() (const value_type) const;

        protected:

          class Fixel : public LineSearchFunctorBase::FixelBase
          {
            public:
            using BaseType = LineSearchFunctorBase::FixelBase;
            Fixel (const SIFT::Track_fixel_contribution&, const TckFactor::Fixel, const value_type, const value_type);
            value_type delta_TD, ddeltaTD_dddelta, mean_delta, delta_FD;
          };

          const value_type weighting_factor;
          const value_type delta;
          vector<Fixel> fixels;

      };







      template <reg_basis_t RegBasis>
      CostAndDerivatives LineSearchFunctorDifferential::get (const value_type ddelta) const
      {
        const value_type deltaplusddelta = delta + ddelta;
        CostAndDerivatives data_result, reg_result;

        for (vector<Fixel>::const_iterator i = fixels.begin(); i != fixels.end(); ++i) {

          const value_type weighted_length = i->length * weighting_factor;
          //const value_type contribution = weighted_length * deltaplusddelta;
          //const value_type scaled_contribution = mu * contribution;

          // TODO Get the data cost terms into here directly
          // Note that fixel TD already has the prior streamline contribution subtracted
          // cost = frac * weight * diff^2
          // diff = (u * (dTD + ((delta+ddelta)*length*factor) + (ddeltaTD_ddelta*ddelta)) - dFD
          const value_type diff = (mu * (i->delta_TD + (deltaplusddelta*weighted_length) + (i->ddeltaTD_dddelta*ddelta))) - i->delta_FD;
          const value_type ddiff_dddelta = mu * (weighted_length + i->ddeltaTD_dddelta);

          // TODO Should fractional attribution of the fixel cost function be modulated by absolute streamline weighting factor?
          data_result.cost += i->cost_frac * i->weight * Math::pow2 (diff);

          // dcost_dddelta = dcost_ddiff * ddiff_dddelta
          //               = (2.0 * frac * weight * diff) * ddiff_dddelta
          data_result.first_deriv += 2.0 * i->cost_frac * i->weight * diff * ddiff_dddelta;

          // d2cost_dddelta2 = d(dcost_dddelta)_ddiff * ddiff_dddelta
          //                 = (2.0 * frac * weight * ddiff_dddelta) * ddiff_dddelta
          data_result.second_deriv += 2.0 * i->cost_frac * i->weight * Math::pow2 (ddiff_dddelta);

          // d3cost_dddelta3 = 0.0

          if (RegBasis == reg_basis_t::FIXEL)
            SIFT2::dxregdelta_ddeltax (reg_result, deltaplusddelta, i->SL_eff, i->mean_delta);

        }

        if (RegBasis == reg_basis_t::STREAMLINE)
          SIFT2::dxregdelta_ddeltax (reg_result, deltaplusddelta, reg_multiplier_streamline);
        else
          reg_result *= reg_multiplier_fixel;

        return CostAndDerivatives(data_result, reg_result);
      }





      template <reg_basis_t RegBasis>
      LineSearchFunctorDifferential::value_type LineSearchFunctorDifferential::operator() (const value_type ddelta) const
      {
        const value_type deltaplusddelta = delta + ddelta;
        value_type cf_data = value_type(0.0);
        value_type cf_reg (RegBasis == reg_basis_t::STREAMLINE ?
                           reg_multiplier_streamline * SIFT2::reg_delta (deltaplusddelta) :
                           value_type(0.0));
        for (vector<Fixel>::const_iterator i = fixels.begin(); i != fixels.end(); ++i) {
          cf_data += i->cost_frac * i->weight * Math::pow2 ((mu * (i->delta_TD + (i->length * weighting_factor * deltaplusddelta) + (i->ddeltaTD_dddelta*ddelta))) - i->delta_FD);
          if (RegBasis == reg_basis_t::FIXEL)
            cf_reg += i->SL_eff * SIFT2::reg_delta (deltaplusddelta, i->mean_delta);
        }
        if (RegBasis == reg_basis_t::FIXEL)
          cf_reg *= reg_multiplier_fixel;
        return (cf_data + cf_reg);
      }






      }
    }
  }
}


#endif

