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


