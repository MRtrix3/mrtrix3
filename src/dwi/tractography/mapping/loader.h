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



