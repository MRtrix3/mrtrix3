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

#include <mutex>

#include "dwi/tractography/SIFT2/fixel_updater.h"
#include "dwi/tractography/SIFT2/tckfactor.h"

#include "dwi/tractography/SIFT/track_contribution.h"




namespace MR {
  namespace DWI {
    namespace Tractography {
      namespace SIFT2 {



      FixelUpdaterBase::FixelUpdaterBase (TckFactor& tckfactor) :
          master (tckfactor),
          local_sum_contributions (tckfactor.nfixels(), 0.0) { }







      FixelUpdaterAbsolute::FixelUpdaterAbsolute (TckFactor& tckfactor) :
          FixelUpdaterBase (tckfactor),
          local_sum_coeffs (tckfactor.nfixels(), 0.0),
          local_counts     (tckfactor.nfixels(), 0) { }



      FixelUpdaterAbsolute::~FixelUpdaterAbsolute()
      {
        std::lock_guard<std::mutex> lock (master.mutex);
        for (MR::Fixel::index_type i = 0; i != master.nfixels(); ++i) {
          TckFactor::Fixel fixel (master, i);
          fixel.add (local_sum_contributions[i], local_counts[i]);
          fixel.add_to_mean_coeff (local_sum_coeffs[i]);
        }
      }



      bool FixelUpdaterAbsolute::operator() (const SIFT::TrackIndexRange& range)
      {
        for (auto track_index : range) {
          const WeightingCoeffAndFactor WCF (WeightingCoeffAndFactor::from_coeff (master.coefficients[track_index]));
          const SIFT::TrackContribution& this_contribution (*(master.contributions[track_index]));
          for (size_t j = 0; j != this_contribution.dim(); ++j) {
            const MR::Fixel::index_type fixel_index = this_contribution[j].get_fixel_index();
            const float length = this_contribution[j].get_length();
            local_sum_contributions[fixel_index] += length * WCF.factor();
            local_sum_coeffs       [fixel_index] += length * WCF.coeff();
            local_counts           [fixel_index]++;
          }
        }
        return true;
      }





      FixelUpdaterDifferential::FixelUpdaterDifferential (TckFactor& tckfactor) :
          FixelUpdaterBase (tckfactor),
          local_sum_deltacoeffs (tckfactor.nfixels(), 0.0) { }




      FixelUpdaterDifferential::~FixelUpdaterDifferential()
      {
        std::lock_guard<std::mutex> lock (master.mutex);
        for (MR::Fixel::index_type i = 0; i != master.nfixels(); ++i) {
          TckFactor::Fixel fixel (master, i);
          fixel.add_to_differential_TD (local_sum_contributions[i]);
          fixel.add_to_mean_deltacoeff(local_sum_deltacoeffs[i]);
        }
      }



      bool FixelUpdaterDifferential::operator() (const SIFT::TrackIndexRange& range)
      {
        for (auto track_index : range) {
          const DifferentialWCF dWCF(DifferentialWCF::from_coeffs(master.coefficients[track_index], master.deltacoeffs[track_index]));
          const SIFT::TrackContribution& this_contribution (*(master.contributions[track_index]));
          for (size_t j = 0; j != this_contribution.dim(); ++j) {
            const MR::Fixel::index_type fixel_index = this_contribution[j].get_fixel_index();
            const float length = this_contribution[j].get_length();
            local_sum_contributions[fixel_index] += dWCF.factor() * length * dWCF.delta_coeff();
            local_sum_deltacoeffs[fixel_index] += length * dWCF.delta_coeff();
          }
        }
        return true;
      }




      }
    }
  }
}


