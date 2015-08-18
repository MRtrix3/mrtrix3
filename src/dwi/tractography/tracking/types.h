/*
    Copyright 2011 Brain Research Institute, Melbourne, Australia

    Written by J-Donald Tournier and Robert E. Smith, 2011.

    This file is part of MRtrix.

    MRtrix is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    MRtrix is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with MRtrix.  If not, see <http://www.gnu.org/licenses/>.

 */

#ifndef __dwi_tractography_tracking_types_h__
#define __dwi_tractography_tracking_types_h__


#include "image/buffer_preload.h"
#include "image/interp/linear.h"



namespace MR
{
  namespace DWI
  {
    namespace Tractography
    {
      namespace Tracking
      {



    enum term_t { CONTINUE, ENTER_CGM, CALIBRATE_FAIL, EXIT_IMAGE, ENTER_CSF, BAD_SIGNAL, HIGH_CURVATURE, LENGTH_EXCEED, TERM_IN_SGM, EXIT_SGM, EXIT_MASK, ENTER_EXCLUDE, TRAVERSE_ALL_INCLUDE };
#define TERMINATION_REASON_COUNT 13

    // This lookup table specifies whether or not the most recent position should be added to the end of the streamline,
    //   based on what mechanism caused the termination
    const uint8_t term_add_to_tck[TERMINATION_REASON_COUNT] = { 1, 1, 0, 0, 1, 0, 0, 1, 0, 0, 0, 1, 1 };

    enum reject_t { TRACK_TOO_SHORT, TRACK_TOO_LONG, ENTER_EXCLUDE_REGION, MISSED_INCLUDE_REGION, ACT_POOR_TERMINATION, ACT_FAILED_WM_REQUIREMENT };
#define REJECTION_REASON_COUNT 6



    typedef Image::BufferPreload<float> SourceBufferType;
    typedef SourceBufferType::value_type value_type;


    template <class VoxelType>
    class Interpolator {
      public:
        typedef Image::Interp::Linear<VoxelType> type;
    };



      }
    }
  }
}

#endif

