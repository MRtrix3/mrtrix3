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

#include "dwi/tractography/SIFT2/line_search.h"
#include "dwi/tractography/SIFT2/regularisation.h"
#include "dwi/tractography/SIFT2/tckfactor.h"


namespace MR {
  namespace DWI {
    namespace Tractography {
      namespace SIFT2 {





      LineSearchFunctor::LineSearchFunctor (const SIFT::track_t index, TckFactor& tckfactor) :
        track_index (index),
        mu (tckfactor.mu()),
        Fs (tckfactor.coefficients[track_index]),
        reg_tik (tckfactor.reg_multiplier_tikhonov),
        // Pre-scale reg_tv by total streamline contribution; each fixel then contributes (PM * length),
        //   and the whole thing is appropriately normalised
        reg_tv  (tckfactor.reg_multiplier_tv / tckfactor.contributions[track_index]->get_total_contribution())
      {
        const SIFT::TrackContribution& track_contribution = *tckfactor.contributions[track_index];
        for (size_t i = 0; i != track_contribution.dim(); ++i) {
          const TckFactor::Fixel fixel (tckfactor, track_contribution[i].get_fixel_index());
          if (!fixel.excluded())
            fixels.push_back (Fixel (track_contribution[i], fixel, Fs, fixel.mean_coeff()));
        }
      }





      LineSearchFunctor::Result
      LineSearchFunctor::get (const value_type dFs) const
      {

        const value_type coefficient = Fs + dFs;
        const value_type factor = std::exp (coefficient);

        Result data_result, tik_result, tv_result;

        for (vector<Fixel>::const_iterator i = fixels.begin(); i != fixels.end(); ++i) {

          const value_type contribution = i->length * factor;
          const value_type scaled_contribution = mu * contribution;
          const value_type roc_contribution = mu * (contribution + i->dTD_dFs);
          const value_type diff = (mu * (i->TD + contribution + (i->dTD_dFs * dFs))) - i->FD;

          data_result.cost += i->cost_frac * i->PM * Math::pow2 (diff);
          data_result.first_deriv += 2.0 * i->PM * i->cost_frac * (roc_contribution * diff);
          data_result.second_deriv += 2.0 * i->PM * i->cost_frac * (Math::pow2 (roc_contribution) + (scaled_contribution * diff));
          data_result.third_deriv += 2.0 * i->PM * i->cost_frac * scaled_contribution * ((3.0*roc_contribution) + diff);

          SIFT2::dxtvreg_dcoeffx (tv_result, coefficient, factor, i->SL_eff, i->meanFs, i->expmeanFs);

        }

        tik_result.cost         = reg_tik * Math::pow2 (coefficient);
        tik_result.first_deriv  = reg_tik * 2.0 * coefficient;
        tik_result.second_deriv = reg_tik * 2.0;

        tv_result *= reg_tv;

        data_result += tik_result;
        data_result += tv_result;
        return data_result;
      }



      LineSearchFunctor::value_type LineSearchFunctor::operator() (const value_type dFs) const
      {
        value_type cf_data = 0.0;
        value_type cf_reg_tv = 0.0;
        for (vector<Fixel>::const_iterator i = fixels.begin(); i != fixels.end(); ++i) {
          cf_data   += i->cost_frac * i->PM * Math::pow2 ((mu * (i->TD + (i->length * std::exp (Fs+dFs)) + (i->dTD_dFs*dFs))) - i->FD);
          cf_reg_tv += i->SL_eff * SIFT2::tvreg (Fs+dFs, i->meanFs);
        }
        const value_type cf_reg_tik = Math::pow2 (Fs+dFs);
        return (cf_data + (reg_tik * cf_reg_tik) + (reg_tv * cf_reg_tv));
      }






      LineSearchFunctor::Fixel::Fixel (const SIFT::Track_fixel_contribution& in, const TckFactor::Fixel fixel, const value_type Fs, const value_type fixel_coeff_mean) :
          index (in.get_fixel_index()),
          length (in.get_length()),
          PM (fixel.weight()),
          TD (fixel.td() - (length * std::exp (Fs))),
          cost_frac (length / fixel.orig_TD()),
          SL_eff (PM * length),
          dTD_dFs ((fixel.orig_TD() - length) * std::exp (Fs)),
          meanFs (fixel_coeff_mean),
          expmeanFs (std::exp (fixel_coeff_mean)),
          FD (fixel.fd()) { }




}
}
}
}

