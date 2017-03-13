/* Copyright (c) 2008-2017 the MRtrix3 contributors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, you can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * MRtrix is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * For more details, see http://www.mrtrix.org/.
 */


#ifndef __dwi_tractography_tracking_types_h__
#define __dwi_tractography_tracking_types_h__


#include "image.h"
#include "interp/linear.h"



namespace MR
{
  namespace DWI
  {
    namespace Tractography
    {
      namespace Tracking
      {



        enum term_t { CONTINUE, ENTER_CGM, CALIBRATOR, EXIT_IMAGE, ENTER_CSF, BAD_SIGNAL, HIGH_CURVATURE, LENGTH_EXCEED, TERM_IN_SGM, EXIT_SGM, EXIT_MASK, ENTER_EXCLUDE, TRAVERSE_ALL_INCLUDE };
#define TERMINATION_REASON_COUNT 13

        // This lookup table specifies whether or not the most recent position should be added to the end of the streamline,
        //   based on what mechanism caused the termination
        const uint8_t term_add_to_tck[TERMINATION_REASON_COUNT] = { 1, 1, 0, 0, 1, 0, 0, 1, 0, 0, 0, 1, 1 };

        enum reject_t { INVALID_SEED, NO_PROPAGATION_FROM_SEED, TRACK_TOO_SHORT, TRACK_TOO_LONG, ENTER_EXCLUDE_REGION, MISSED_INCLUDE_REGION, ACT_POOR_TERMINATION, ACT_FAILED_WM_REQUIREMENT };
#define REJECTION_REASON_COUNT 8



        template <class ImageType>
          class Interpolator { MEMALIGN(Interpolator<ImageType>)
            public:
              typedef Interp::Linear<ImageType> type;
          };



      }
    }
  }
}

#endif

