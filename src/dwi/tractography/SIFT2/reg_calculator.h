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

#ifndef __dwi_tractography_sift2_reg_calculator_h__
#define __dwi_tractography_sift2_reg_calculator_h__


#include "dwi/tractography/SIFT/track_index_range.h"
#include "dwi/tractography/SIFT/types.h"

#include "dwi/tractography/SIFT2/regularisation.h"


namespace MR {
  namespace DWI {
    namespace Tractography {
      namespace SIFT2 {


      class TckFactor;


      class RegularisationCalculator
      { NOMEMALIGN

        public:
          RegularisationCalculator (TckFactor&, double&, double&);
          ~RegularisationCalculator();

          bool operator() (const SIFT::TrackIndexRange& range);


        private:
          TckFactor& master;
          double& cf_reg_tik;
          double& cf_reg_tv;

          // Each thread needs a local copy of these
          double tikhonov_sum, tv_sum;

      };



      }
    }
  }
}



#endif

