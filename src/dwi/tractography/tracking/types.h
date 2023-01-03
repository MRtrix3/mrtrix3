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

#ifndef __dwi_tractography_tracking_types_h__
#define __dwi_tractography_tracking_types_h__


#include "image.h"
#include "interp/linear.h"
#include "interp/masked.h"



namespace MR
{
  namespace DWI
  {
    namespace Tractography
    {
      namespace Tracking
      {



        enum term_t { CONTINUE, ENTER_CGM, CALIBRATOR, EXIT_IMAGE, ENTER_CSF, MODEL, HIGH_CURVATURE, LENGTH_EXCEED, TERM_IN_SGM, EXIT_SGM, EXIT_MASK, ENTER_EXCLUDE, TRAVERSE_ALL_INCLUDE };
#define TERMINATION_REASON_COUNT 13

        // This lookup table specifies whether or not the most recent position should be added to the end of the streamline,
        //   based on what mechanism caused the termination
        const uint8_t term_add_to_tck[TERMINATION_REASON_COUNT] = { 1, 1, 0, 0, 1, 0, 0, 1, 0, 0, 0, 1, 1 };

        enum reject_t { INVALID_SEED, NO_PROPAGATION_FROM_SEED, TRACK_TOO_SHORT, TRACK_TOO_LONG, ENTER_EXCLUDE_REGION, MISSED_INCLUDE_REGION, ACT_POOR_TERMINATION, ACT_FAILED_WM_REQUIREMENT };
#define REJECTION_REASON_COUNT 8



        template <class ImageType>
          class Interpolator { MEMALIGN(Interpolator<ImageType>)
            public:
              using type = Interp::Masked<Interp::Linear<ImageType>>;
          };



      }
    }
  }
}

#endif

