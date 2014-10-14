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
#include "ptr.h"
#include "thread_queue.h"
#include "dwi/tractography/file.h"
#include "dwi/tractography/streamline.h"


namespace MR {
namespace DWI {
namespace Tractography {
namespace Mapping {



class TrackLoader
{

  public:
    TrackLoader (Tractography::Reader<float>& file, const size_t to_load = 0, const std::string& msg = "mapping tracks to image...") :
      reader (file),
      tracks_to_load (to_load),
      progress (msg.size() ? new ProgressBar (msg, tracks_to_load) : NULL)
    { }

    virtual ~TrackLoader() { }
    virtual bool operator() (Streamline<float>& out)
    {
      if (!reader (out)) {
        progress = NULL;
        return false;
      }
      if (tracks_to_load && out.index >= tracks_to_load) {
        out.clear();
        progress = NULL;
        return false;
      }
      if (progress)
        ++(*progress);
      return true;
    }

  protected:
    Tractography::Reader<float>& reader;
    const size_t tracks_to_load;
    Ptr<ProgressBar> progress;

};


}
}
}
}

#endif



