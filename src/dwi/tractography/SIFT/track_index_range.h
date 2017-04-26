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


#ifndef __dwi_tractography_sift_track_index_range_h__
#define __dwi_tractography_sift_track_index_range_h__

#include "progressbar.h"
#include "dwi/tractography/SIFT/types.h"

namespace MR
{
  namespace DWI
  {
    namespace Tractography
    {
      namespace SIFT
      {



#define SIFT_TRACK_INDEX_BUFFER_SIZE 10000



      using TrackIndexRange = std::pair<track_t, track_t>;
      using TrackIndexRangeQueue = Thread::Queue< TrackIndexRange >;



      // Some processes in SIFT are fast for each streamline, but there are a large number of streamlines, so
      //   if multi-threading is done on a per-track basis the I/O associated with multi-threading begins to dominate
      // Instead, the input queue for multi-threading is filled with std::pair<track_t, track_t>'s, where the values
      //   are the start and end track indices to be processed
      class TrackIndexRangeWriter
      { MEMALIGN(TrackIndexRangeWriter)

        public:
          TrackIndexRangeWriter (const track_t, const track_t, const std::string& message = std::string ());

          bool operator() (TrackIndexRange&);

        private:
          const track_t size, end;
          track_t start;
          std::unique_ptr<ProgressBar> progress;

      };




      }
    }
  }
}


#endif

