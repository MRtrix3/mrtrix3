/* Copyright (c) 2008-2017 the MRtrix3 contributors.
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


#ifndef __dwi_tractography_mapping_loader_h__
#define __dwi_tractography_mapping_loader_h__


#include "memory.h"
#include "progressbar.h"
#include "thread_queue.h"
#include "dwi/tractography/file.h"
#include "dwi/tractography/streamline.h"


namespace MR {
  namespace DWI {
    namespace Tractography {
      namespace Mapping {



        class TrackLoader
        { MEMALIGN(TrackLoader)

          public:
            TrackLoader (Reader<>& file, const size_t to_load = 0, const std::string& msg = "mapping tracks to image") :
              reader (file),
              tracks_to_load (to_load),
              progress (msg.size() ? new ProgressBar (msg, tracks_to_load) : nullptr) { }

            virtual ~TrackLoader() { }
            virtual bool operator() (Streamline<>& out)
            {
              if (!reader (out)) {
                progress.reset();
                return false;
              }
              if (tracks_to_load && out.index >= tracks_to_load) {
                out.clear();
                progress.reset();
                return false;
              }
              if (progress)
                ++(*progress);
              return true;
            }

          protected:
            Reader<>& reader;
            const size_t tracks_to_load;
            std::unique_ptr<ProgressBar> progress;

        };


      }
    }
  }
}

#endif



