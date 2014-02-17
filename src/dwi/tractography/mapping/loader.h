#ifndef __dwi_tractography_mapping_loader_h__
#define __dwi_tractography_mapping_loader_h__


#include "progressbar.h"
#include "ptr.h"
#include "thread/queue.h"
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



