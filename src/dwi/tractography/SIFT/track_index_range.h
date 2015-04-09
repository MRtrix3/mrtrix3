/*
    Copyright 2011 Brain Research Institute, Melbourne, Australia

    Written by Robert Smith, 2011.

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



#ifndef __dwi_tractography_sift_track_index_range_h__
#define __dwi_tractography_sift_track_index_range_h__


#include "progressbar.h"
#include "ptr.h"

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



      typedef std::pair<track_t, track_t> TrackIndexRange;
      typedef Thread::Queue< TrackIndexRange > TrackIndexRangeQueue;



      // Some processes in SIFT are fast for each streamline, but there are a large number of streamlines, so
      //   if multi-threading is done on a per-track basis the I/O associated with multi-threading begins to dominate
      // Instead, the input queue for multi-threading is filled with std::pair<track_t, track_t>'s, where the values
      //   are the start and end track indices to be processed
      class TrackIndexRangeWriter
      {

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

