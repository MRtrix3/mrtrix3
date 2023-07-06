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





      LineSearchFunctor::LineSearchFunctor (const SIFT::track_t index, TckFactor& tckfactor) :
        track_index (index),
        mu (tckfactor.mu()),
        Fs (tckfactor.coefficients[track_index]),
        reg_multiplier_streamline (tckfactor.reg_multiplier),
        reg_multipler_fixel (tckfactor.reg_multiplier / tckfactor.contributions[track_index]->get_total_contribution())
      {
        const SIFT::TrackContribution& track_contribution = *tckfactor.contributions[track_index];
        for (size_t i = 0; i != track_contribution.dim(); ++i) {
          const TckFactor::Fixel fixel (tckfactor, track_contribution[i].get_fixel_index());
          if (!fixel.excluded())
            fixels.push_back (Fixel (track_contribution[i], fixel, Fs, fixel.mean_coeff()));
        }
      }











      LineSearchFunctor::Fixel::Fixel (const SIFT::Track_fixel_contribution& in, const TckFactor::Fixel fixel, const value_type Fs, const value_type fixel_coeff_mean) :
          index (in.get_fixel_index()),
          length (in.get_length()),
          weight (fixel.weight()),
          TD (fixel.td() - (length * std::exp (Fs))),
          cost_frac (length / fixel.orig_TD()),
          SL_eff (weight * length),
          dTD_dFs ((fixel.orig_TD() - length) * std::exp (Fs)),
          meanFs (fixel_coeff_mean),
          expmeanFs (std::exp (fixel_coeff_mean)),
          FD (fixel.fd()) { }








}
}
}
}

