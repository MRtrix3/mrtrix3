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



#include "dwi/tractography/SIFT/track_index_range.h"


namespace MR
{
  namespace DWI
  {
    namespace Tractography
    {
      namespace SIFT
      {



      TrackIndexRangeWriter::TrackIndexRangeWriter (const track_t buffer_size, const track_t num_tracks, const std::string& message) :
        size  (buffer_size),
        end   (num_tracks),
        start (0),
        progress (message.empty() ? NULL : new ProgressBar (message, ceil (float(end) / float(size)))) { }


      bool TrackIndexRangeWriter::operator() (TrackIndexRange& out)
      {
        if (start >= end)
          return false;
        out.first = start;
        const track_t last = std::min (start + size, end);
        out.second = last;
        start = last;
        if (progress)
          ++*progress;
        return true;
      }



      }
    }
  }
}


