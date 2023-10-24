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
#include "dwi/tractography/SIFT2/line_search.h"
#include "dwi/tractography/SIFT2/tckfactor.h"


namespace MR {
  namespace DWI {
    namespace Tractography {
      namespace SIFT2 {






      LineSearchFunctorBase::LineSearchFunctorBase (const SIFT::track_t index, TckFactor& tckfactor) :
        track_index (index),
        mu (tckfactor.mu()),
        reg_multiplier_streamline (std::numeric_limits<value_type>::signaling_NaN()),
        reg_multiplier_fixel (std::numeric_limits<value_type>::signaling_NaN()) { }






      LineSearchFunctorAbsolute::LineSearchFunctorAbsolute (const SIFT::track_t index, TckFactor& tckfactor) :
          LineSearchFunctorBase (index, tckfactor),
          Fs (tckfactor.coefficients[track_index])
      {
        reg_multiplier_streamline = tckfactor.reg_multiplier_abs;
        reg_multiplier_fixel = tckfactor.reg_multiplier_abs / tckfactor.contributions[track_index]->get_total_contribution();
        const SIFT::TrackContribution& track_contribution = *tckfactor.contributions[track_index];
        const WeightingCoeffAndFactor WCF_streamline (WeightingCoeffAndFactor::from_coeff (Fs));
        for (size_t i = 0; i != track_contribution.dim(); ++i) {
          const TckFactor::Fixel fixel (tckfactor, track_contribution[i].get_fixel_index());
          if (!fixel.excluded())
            fixels.push_back (Fixel (track_contribution[i], fixel, WCF_streamline, WeightingCoeffAndFactor::from_coeff (fixel.mean_coeff())));
        }
      }





      LineSearchFunctorBase::FixelBase::FixelBase (const SIFT::Track_fixel_contribution& in,
                                                   const TckFactor::Fixel fixel) :
          index (in.get_fixel_index()),
          length (in.get_length()),
          weight (fixel.weight()),
          cost_frac (length / fixel.orig_TD()),
          SL_eff (weight * length) { }






      LineSearchFunctorAbsolute::Fixel::Fixel (const SIFT::Track_fixel_contribution& in,
                                               const TckFactor::Fixel fixel,
                                               const WeightingCoeffAndFactor& WCF_streamline,
                                               const WeightingCoeffAndFactor& WCF_fixel) :
          BaseType (in, fixel),
          TD (fixel.td() - (length * WCF_streamline.factor())),
          dTD_dFs ((fixel.orig_TD() - length) * WCF_streamline.factor()),
          FD (fixel.fd()),
          WCF (WCF_fixel) { }






      LineSearchFunctorDifferential::LineSearchFunctorDifferential (const SIFT::track_t index, TckFactor& tckfactor) :
          LineSearchFunctorBase (index, tckfactor),
          weighting_factor (WeightingCoeffAndFactor::from_coeff (tckfactor.coefficients[index]).factor()),
          delta (tckfactor.deltas[index])
      {
        reg_multiplier_streamline = tckfactor.reg_multiplier_diff;
        reg_multiplier_fixel = tckfactor.reg_multiplier_diff / tckfactor.contributions[track_index]->get_total_contribution();
        const SIFT::TrackContribution& track_contribution = *tckfactor.contributions[track_index];
        for (size_t i = 0; i != track_contribution.dim(); ++i) {
          const TckFactor::Fixel fixel (tckfactor, track_contribution[i].get_fixel_index());
          if (!fixel.excluded())
            fixels.push_back (Fixel (track_contribution[i], fixel, weighting_factor, delta));
        }
      }





      LineSearchFunctorDifferential::Fixel::Fixel (const SIFT::Track_fixel_contribution& in,
                                            const TckFactor::Fixel fixel,
                                            const value_type weighting_factor,
                                            const value_type delta) :
          BaseType (in, fixel),
          delta_TD (fixel.delta_TD() - (length * weighting_factor * delta)),
          // TODO Am I confident that this is what should be used?
          ddeltaTD_dddelta ((fixel.orig_TD() - length * weighting_factor) * weighting_factor),
          mean_delta (fixel.mean_delta()),
          delta_FD (fixel.delta_FD()) { }




}
}
}
}

