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


namespace MR {
namespace DWI {
namespace Tractography {
namespace Mapping {


template <class QueueType>
class TrackLoader
{

  public:
    TrackLoader (QueueType& queue, DWI::Tractography::Reader<float>& file, const size_t count) :
      writer (queue),
      reader (file),
      total_count (count) { }

    virtual void execute ()
    {
      typename QueueType::Writer::Item item (writer);

      ProgressBar progress ("mapping tracks to image...", total_count);
      while (reader.next (*item)) {
        if (!item.write())
          throw Exception ("error writing to track-mapping queue");
        ++progress;
      }
    }

  protected:
    typename QueueType::Writer writer;
    DWI::Tractography::Reader<float>& reader;
    size_t total_count;

};


}
}
}
}

#endif



