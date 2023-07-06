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




namespace MR {
  namespace DWI {
    namespace Tractography {
      namespace SIFT2 {



      class TckFactor;



      // TODO New templated version of LineSearchFunctor that will deal with new regularisation templates
      // TODO Consider templating maximal required derivative order?
      // TODO Can we avoid templating the class and just template the functors?
      //   - Worried that this will have downsides...
      //template <reg_basis_t RegBasis, reg_fn_t RegFn>
      class LineSearchFunctor
      {
        public:
          using value_type = SIFT::value_type;

          LineSearchFunctor (const SIFT::track_t, TckFactor&);

          template <reg_basis_t RegBasis, reg_fn_t RegFn>
          CostAndDerivatives get (const value_type) const;
          template <reg_basis_t RegBasis, reg_fn_t RegFn>
          value_type operator() (const value_type) const;

        protected:

          // Necessary information for each of those fixels traversed by this streamline
          class Fixel
          {
            public:
            Fixel (const SIFT::Track_fixel_contribution&, const TckFactor::Fixel, const value_type, const value_type);
            MR::Fixel::index_type index;
            value_type length, weight, TD, cost_frac, SL_eff, dTD_dFs, meanFs, expmeanFs, FD;
          };

          const SIFT::track_t track_index;
          const value_type mu;
          const value_type Fs;
          const value_type reg_multiplier_streamline;
          const value_type reg_multipler_fixel;

          vector<Fixel> fixels;

      };




      template <reg_basis_t RegBasis, reg_fn_t RegFn>
      CostAndDerivatives
      LineSearchFunctor::get (const value_type dFs) const
      {

        const value_type coefficient = Fs + dFs;
        const value_type factor = std::exp (coefficient);

        CostAndDerivatives data_result, reg_result;

        for (vector<Fixel>::const_iterator i = fixels.begin(); i != fixels.end(); ++i) {

          const value_type contribution = i->length * factor;
          const value_type scaled_contribution = mu * contribution;
          const value_type roc_contribution = mu * (contribution + i->dTD_dFs);
          const value_type diff = (mu * (i->TD + contribution + (i->dTD_dFs * dFs))) - i->FD;

          data_result.cost += i->cost_frac * i->weight * Math::pow2 (diff);
          data_result.first_deriv += 2.0 * i->weight * i->cost_frac * (roc_contribution * diff);
          data_result.second_deriv += 2.0 * i->weight * i->cost_frac * (Math::pow2 (roc_contribution) + (scaled_contribution * diff));
          data_result.third_deriv += 2.0 * i->weight * i->cost_frac * scaled_contribution * ((3.0*roc_contribution) + diff);

          if (RegBasis == reg_basis_t::FIXEL)
            SIFT2::dxreg_dcoeffx<RegFn> (reg_result, coefficient, factor, i->SL_eff, i->meanFs, i->expmeanFs);

        }

        if (RegBasis == reg_basis_t::STREAMLINE)
          SIFT2::dxreg_dcoeffx<RegFn> (reg_result, coefficient, factor, reg_multiplier_streamline);
        else
          reg_result *= reg_multipler_fixel;

        return CostAndDerivatives(data_result, reg_result);
      }




      template <reg_basis_t RegBasis, reg_fn_t RegFn>
      LineSearchFunctor::value_type LineSearchFunctor::operator() (const value_type dFs) const
      {
        value_type cf_data = value_type(0.0);
        value_type cf_reg (RegBasis == reg_basis_t::STREAMLINE ?
                           reg_multiplier_streamline * SIFT2::reg<RegFn> (Fs+dFs) :
                           value_type(0.0));
        for (vector<Fixel>::const_iterator i = fixels.begin(); i != fixels.end(); ++i) {
          cf_data += i->cost_frac * i->weight * Math::pow2 ((mu * (i->TD + (i->length * std::exp (Fs+dFs)) + (i->dTD_dFs*dFs))) - i->FD);
          if (RegBasis == reg_basis_t::FIXEL)
            cf_reg += i->SL_eff * SIFT2::reg<RegFn> (Fs+dFs, i->meanFs);
        }
        if (RegBasis == reg_basis_t::FIXEL)
          cf_reg *= reg_multipler_fixel;
        return (cf_data + cf_reg);
      }





      }
    }
  }
}


#endif

