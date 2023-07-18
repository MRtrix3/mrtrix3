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


