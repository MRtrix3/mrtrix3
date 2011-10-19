/*
    Copyright 2011 Brain Research Institute, Melbourne, Australia

    Written by Robert E. Smith, 2011.

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

#ifndef __dwi_tractography_mapping_loader_h__
#define __dwi_tractography_mapping_loader_h__


#include "progressbar.h"
#include "thread/queue.h"
#include "dwi/tractography/file.h"

#include "dwi/tractography/mapping/common.h"


namespace MR {
namespace DWI {
namespace Tractography {
namespace Mapping {



class TrackLoader
{

  public:
    TrackLoader (TrackQueue& queue, DWI::Tractography::Reader<float>& file, const size_t count) :
      writer (queue),
      reader (file),
      total_count (count) { }

    virtual void execute ()
    {
      TrackQueue::Writer::Item item (writer);
      size_t counter = 0;

      ProgressBar progress ("mapping tracks to image...", total_count);
      while (reader.next (item->tck)) {
        item->index = counter++;
        if (!item.write())
          throw Exception ("error writing to track-mapping queue");
        ++progress;
      }
    }

  protected:
    TrackQueue::Writer writer;
    DWI::Tractography::Reader<float>& reader;
    size_t total_count;

};


}
}
}
}

#endif



