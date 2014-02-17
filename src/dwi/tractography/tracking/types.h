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


        enum term_t { CONTINUE, CALIBRATE_FAIL, EXIT_IMAGE, BAD_SIGNAL, HIGH_CURVATURE, LENGTH_EXCEED, EXIT_MASK, ENTER_EXCLUDE };
#define TERMINATION_REASON_COUNT 8

        // This lookup table specifies whether or not the most recent position should be added to the end of the streamline,
        //   based on what mechanism caused the termination
        const uint8_t term_add_to_tck[TERMINATION_REASON_COUNT] = { 1, 0, 1, 0, 0, 0, 0, 1 };

        enum reject_t { TRACK_TOO_SHORT, ENTER_EXCLUDE_REGION, MISSED_INCLUDE_REGION };
#define REJECTION_REASON_COUNT 3


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

