/*******************************************************************************
    Copyright (C) 2014 Brain Research Institute, Melbourne, Australia
    
    Permission is hereby granted under the Patent Licence Agreement between
    the BRI and Siemens AG from July 3rd, 2012, to Siemens AG obtaining a
    copy of this software and associated documentation files (the
    "Software"), to deal in the Software without restriction, including
    without limitation the rights to possess, use, develop, manufacture,
    import, offer for sale, market, sell, lease or otherwise distribute
    Products, and to permit persons to whom the Software is furnished to do
    so, subject to the following conditions:
    
    The above copyright notice and this permission notice shall be included
    in all copies or substantial portions of the Software.
    
    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
    OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
    MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
    IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
    CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
    TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
    SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

*******************************************************************************/


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

