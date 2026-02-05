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
          value_type reg_multiplier_streamline; // Also used for streamline-group-wise regularisation
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
          template <reg_basis_t RegBasis, reg_fn_abs_t RegFn>
          CostAndDerivatives get (const value_type) const;
          template <reg_basis_t RegBasis, reg_fn_abs_t RegFn>
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
          const WeightingCoeffAndFactor group_mean;
          vector<Fixel> fixels;

      };



      // TODO Move to .cpp
      template <reg_basis_t RegBasis, reg_fn_abs_t RegFn>
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

        switch (RegBasis) {
          case reg_basis_t::STREAMLINE:
            SIFT2::dxreg_dcoeffx<RegFn> (reg_result, WCF, reg_multiplier_streamline);
            break;
          case reg_basis_t::FIXEL:
            reg_result *= reg_multiplier_fixel;
            break;
          case reg_basis_t::GROUP:
            SIFT2::dxreg_dcoeffx<RegFn> (reg_result, WCF, reg_multiplier_streamline, group_mean);
            break;
        }

        return CostAndDerivatives(data_result, reg_result);
      }




      template <reg_basis_t RegBasis, reg_fn_abs_t RegFn>
      LineSearchFunctorAbsolute::value_type LineSearchFunctorAbsolute::operator() (const value_type dFs) const
      {
        const WeightingCoeffAndFactor WCF (WeightingCoeffAndFactor::from_coeff (Fs+dFs));
        value_type cf_data (value_type(0));
        value_type cf_reg (value_type(0));
        for (vector<Fixel>::const_iterator i = fixels.begin(); i != fixels.end(); ++i) {
          cf_data += i->cost_frac * i->weight * Math::pow2 ((mu * (i->TD + (i->length * WCF.factor()) + (i->dTD_dFs*dFs))) - i->FD);
          if (RegBasis == reg_basis_t::FIXEL)
            cf_reg += i->SL_eff * SIFT2::reg<RegFn> (WCF, i->WCF);
        }
        switch (RegBasis) {
          case reg_basis_t::STREAMLINE:
            cf_reg = reg_multiplier_streamline * SIFT2::reg<RegFn> (WCF);
            break;
          case reg_basis_t::FIXEL:
            cf_reg *= reg_multiplier_fixel;
            break;
          case reg_basis_t::GROUP:
            cf_reg = reg_multiplier_streamline * SIFT2::reg<RegFn> (WCF, group_mean);
            break;
        }
        return (cf_data + cf_reg);
      }












      class LineSearchFunctorDifferential : public LineSearchFunctorBase
      {
        public:
          LineSearchFunctorDifferential (const SIFT::track_t, TckFactor&);

          template <reg_basis_t RegBasis, reg_fn_diff_t RegFn>
          CostAndDerivatives get (const value_type) const;
          // template <reg_basis_t RegBasis, reg_fn_diff_t RegFn>
          // CostAndDerivatives get_verbose (const value_type) const;
          template <reg_basis_t RegBasis, reg_fn_diff_t RegFn>
          value_type operator() (const value_type) const;
          // template <reg_basis_t RegBasis, reg_fn_diff_t RegFn>
          // value_type cost_verbose (const value_type) const;

        protected:

          class Fixel : public LineSearchFunctorBase::FixelBase
          {
            public:
            using BaseType = LineSearchFunctorBase::FixelBase;
            Fixel (const SIFT::Track_fixel_contribution&, const TckFactor::Fixel, const DifferentialWCF &);
            value_type differential_TD, ddiffTD_dddelta, mean_deltacoeff, differential_FD;
          };

          // bool verbose;
          const DifferentialWCF base_dWCF;
          const value_type group_mean;
          vector<Fixel> fixels;

      };







      template <reg_basis_t RegBasis, reg_fn_diff_t RegFn>
      CostAndDerivatives LineSearchFunctorDifferential::get (const value_type ddeltacoeff) const
      {
//         if (verbose)
//           return get_verbose<RegBasis, RegFn>(ddeltacoeff);
        const DifferentialWCF dWCF (DifferentialWCF::from_deltacoeff(base_dWCF.absolute(), base_dWCF.delta_coeff() + ddeltacoeff));
        CostAndDerivatives data_result, reg_result;

        for (vector<Fixel>::const_iterator i = fixels.begin(); i != fixels.end(); ++i) {

          // Get the data cost terms into here directly
          // Note different expression to absolute version
          const value_type diff = (mu * (i->differential_TD + (i->ddiffTD_dddelta*ddeltacoeff))) - i->differential_FD;

          // TODO Should fractional attribution of the fixel cost function be modulated by absolute streamline weighting factor?
          data_result.cost += i->cost_frac * i->weight * Math::pow2 (diff);
          data_result.first_deriv += 2.0 * i->cost_frac * i->weight * mu * i->ddiffTD_dddelta * diff;
          data_result.second_deriv += 2.0 * i->cost_frac * i->weight * Math::pow2(mu) * Math::pow2(i->ddiffTD_dddelta);
          // data_result.third_deriv = 0.0

          if (RegBasis == reg_basis_t::FIXEL)
            SIFT2::dxreg_ddeltacoeffx<RegFn> (reg_result, dWCF, i->SL_eff, i->mean_deltacoeff);

        }

        switch (RegBasis) {
          case reg_basis_t::STREAMLINE:
            SIFT2::dxreg_ddeltacoeffx<RegFn> (reg_result, dWCF, reg_multiplier_streamline);
            break;
          case reg_basis_t::FIXEL:
            reg_result *= reg_multiplier_fixel;
            break;
          case reg_basis_t::GROUP:
            SIFT2::dxreg_ddeltacoeffx<RegFn> (reg_result, dWCF, reg_multiplier_streamline, group_mean);
            break;
        }

        return CostAndDerivatives(data_result, reg_result);
      }

/*
      template <reg_basis_t RegBasis, reg_fn_diff_t RegFn>
      CostAndDerivatives LineSearchFunctorDifferential::get_verbose (const value_type ddeltacoeff) const
      {
        std::cerr << "New get() call for streamline of interest\n";
        std::cerr << "Baseline delta coeff: " << base_dWCF.delta_coeff() << "\n";
        std::cerr << "Candidate change: " << ddeltacoeff << "\n";
        const DifferentialWCF dWCF (DifferentialWCF::from_deltacoeff(base_dWCF.absolute(), base_dWCF.delta_coeff() + ddeltacoeff));
        std::cerr << "Modified delta coeff: " << dWCF.delta_coeff() << "\n";
        CostAndDerivatives data_result, reg_result;

        for (vector<Fixel>::const_iterator i = fixels.begin(); i != fixels.end(); ++i) {

          // Get the data cost terms into here directly
          // Note different expression to absolute version
          const value_type diff = (mu * (i->differential_TD + (i->ddiffTD_dddelta*ddeltacoeff))) - i->differential_FD;

          CostAndDerivatives this_data_result;
          this_data_result.cost += i->cost_frac * i->weight * Math::pow2 (diff);
          this_data_result.first_deriv += 2.0 * i->cost_frac * i->weight * mu * i->ddiffTD_dddelta * diff;
          this_data_result.second_deriv += 2.0 * i->cost_frac * i->weight * Math::pow2(mu) * Math::pow2(i->ddiffTD_dddelta);
          // data_result.third_deriv = 0.0
          std::cerr << "Fixel data term (" << i->index << "): Frac=" << i->cost_frac << "; weight=" << i->weight << "; cost=" << this_data_result.cost << "\n";
          data_result += this_data_result;

          CostAndDerivatives this_reg_result;
          if (RegBasis == reg_basis_t::FIXEL) {
            SIFT2::dxreg_ddeltacoeffx<RegFn> (this_reg_result, dWCF, i->SL_eff, i->mean_deltacoeff);
            std::cerr << "Fixel reg term (" << i->index << "): mean=" << i->mean_deltacoeff << "; cost=" << this_reg_result.cost << "\n";
            reg_result += this_reg_result;
          }

        }

        switch (RegBasis) {
          case reg_basis_t::STREAMLINE:
            SIFT2::dxreg_ddeltacoeffx<RegFn> (reg_result, dWCF, reg_multiplier_streamline);
            break;
          case reg_basis_t::FIXEL:
            reg_result *= reg_multiplier_fixel;
            break;
          case reg_basis_t::GROUP:
            SIFT2::dxreg_ddeltacoeffx<RegFn> (reg_result, dWCF, reg_multiplier_streamline, group_mean);
            break;
        }

        const CostAndDerivatives result(data_result, reg_result);
        std::cerr << "Final result: " << result << "\n";
        return result;
      }
*/




      template <reg_basis_t RegBasis, reg_fn_diff_t RegFn>
      LineSearchFunctorDifferential::value_type LineSearchFunctorDifferential::operator() (const value_type ddeltacoeff) const
      {
//         if (verbose)
//           return cost_verbose<RegBasis, RegFn>(ddeltacoeff);
        const value_type deltaplusddelta = base_dWCF.delta_coeff() + ddeltacoeff;
        value_type cf_data (value_type(0));
        value_type cf_reg (RegBasis == reg_basis_t::STREAMLINE ?
                           reg_multiplier_streamline * SIFT2::reg<RegFn> (deltaplusddelta) :
                           value_type(0.0));
        for (vector<Fixel>::const_iterator i = fixels.begin(); i != fixels.end(); ++i) {
          cf_data += i->cost_frac * i->weight * Math::pow2 ((mu * (i->differential_TD + i->ddiffTD_dddelta*ddeltacoeff)) - i->differential_FD);
          if (RegBasis == reg_basis_t::FIXEL)
            cf_reg += i->SL_eff * SIFT2::reg<RegFn> (deltaplusddelta, i->mean_deltacoeff);
        }
        switch (RegBasis) {
          case reg_basis_t::STREAMLINE:
            cf_reg = reg_multiplier_streamline * SIFT2::reg<RegFn> (deltaplusddelta);
            break;
          case reg_basis_t::FIXEL:
            cf_reg *= reg_multiplier_fixel;
            break;
          case reg_basis_t::GROUP:
            cf_reg = reg_multiplier_streamline * SIFT2::reg<RegFn> (deltaplusddelta, group_mean);
            break;
        }
        return (cf_data + cf_reg);
      }

/*
      template <reg_basis_t RegBasis, reg_fn_diff_t RegFn>
      LineSearchFunctorDifferential::value_type LineSearchFunctorDifferential::cost_verbose (const value_type ddeltacoeff) const
      {
        std::cerr << "New get() call for streamline of interest\n";
        std::cerr << "Baseline delta coeff: " << base_dWCF.delta_coeff() << "\n";
        std::cerr << "Candidate change: " << ddeltacoeff << "\n";
        const value_type deltaplusddelta = base_dWCF.delta_coeff() + ddeltacoeff;
        std::cerr << "New delta coeff: " << deltaplusddelta << "\n";
        value_type cf_data (value_type(0));
        value_type cf_reg (RegBasis == reg_basis_t::STREAMLINE ?
                           reg_multiplier_streamline * SIFT2::reg<RegFn> (deltaplusddelta) :
                           value_type(0.0));
        for (vector<Fixel>::const_iterator i = fixels.begin(); i != fixels.end(); ++i) {
          const value_type this_cf_data = i->cost_frac * i->weight * Math::pow2 ((mu * (i->differential_TD + i->ddiffTD_dddelta*ddeltacoeff)) - i->differential_FD);
          std::cerr << "Fixel data term (" << i->index << "): Frac=" << i->cost_frac << "; weight=" << i->weight << "; cost=" << this_cf_data << "\n";
          cf_data += this_cf_data;
          if (RegBasis == reg_basis_t::FIXEL) {
            const value_type this_cf_reg = i->SL_eff * SIFT2::reg<RegFn> (deltaplusddelta, i->mean_deltacoeff);
            std::cerr << "Fixel reg term (" << i->index << "): mean=" << i->mean_deltacoeff << "; cost=" << this_cf_reg << "\n";
            cf_reg += this_cf_reg;
          }
        }
        switch (RegBasis) {
          case reg_basis_t::STREAMLINE:
            cf_reg = reg_multiplier_streamline * SIFT2::reg<RegFn> (deltaplusddelta);
            break;
          case reg_basis_t::FIXEL:
            cf_reg *= reg_multiplier_fixel;
            break;
          case reg_basis_t::GROUP:
            cf_reg = reg_multiplier_streamline * SIFT2::reg<RegFn> (deltaplusddelta, group_mean);
            break;
        }
        const value_type result = cf_data + cf_reg;
        std::cerr << "Final result: " << result << "\n";
        return result;
      }
*/





      }
    }
  }
}


#endif

