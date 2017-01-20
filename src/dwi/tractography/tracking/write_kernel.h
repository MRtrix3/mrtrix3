/*
 * Copyright (c) 2008-2016 the MRtrix3 contributors
 * 
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/
 * 
 * MRtrix is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * 
 * For more details, see www.mrtrix.org
 * 
 */

#ifndef __dwi_tractography_tracking_write_kernel_h__
#define __dwi_tractography_tracking_write_kernel_h__

#include <string>
#include <vector>
#include <cinttypes>

#include "timer.h"
#include "file/ofstream.h"

#include "dwi/tractography/file.h"
#include "dwi/tractography/properties.h"
#include "dwi/tractography/streamline.h"

#include "dwi/tractography/tracking/early_exit.h"
#include "dwi/tractography/tracking/generated_track.h"
#include "dwi/tractography/tracking/shared.h"
#include "dwi/tractography/tracking/types.h"



namespace MR
{
  namespace DWI
  {
    namespace Tractography
    {
      namespace Tracking
      {


      class WriteKernel
      { MEMALIGN(WriteKernel)
        public:

          WriteKernel (const SharedBase& shared,
              const std::string& output_file,
              const DWI::Tractography::Properties& properties) :
                S (shared),
                writer (output_file, properties),
                always_increment (S.properties.seeds.is_finite() || !S.max_num_tracks),
                warn_on_max_attempts (S.implicit_max_num_attempts),
                progress (printf ("       0 generated,        0 selected", 0, 0), always_increment ? S.max_num_attempts : S.max_num_tracks),
                early_exit (shared)
          {
            const auto seed_output = properties.find ("seed_output");
            if (seed_output != properties.end()) {
              seeds.reset (new File::OFStream (seed_output->second, std::ios_base::out | std::ios_base::trunc));
              (*seeds) << "#Track_index,Seed_index,Pos_x,Pos_y,Pos_z,\n";
            }
          }

          WriteKernel (const WriteKernel&) = delete;
          WriteKernel& operator= (const WriteKernel&) = delete;

          ~WriteKernel ()
          {
            // Use set_text() rather than update() here to force update of the text before progress goes out of scope
            progress.set_text (printf ("%8" PRIu64 " generated, %8" PRIu64 " selected", writer.total_count, writer.count));
            if (warn_on_max_attempts && writer.total_count == S.max_num_attempts
                && S.max_num_tracks && writer.count < S.max_num_tracks) {
              WARN ("less than desired streamline number due to implicit maximum number of attempts; set -maxnum 0 to override");
            }
            if (seeds) {
              (*seeds) << "\n";
              seeds->close();
            }

          }


          bool operator() (const GeneratedTrack&);

          bool complete() const { return ((S.max_num_tracks && writer.count >= S.max_num_tracks) || (S.max_num_attempts && writer.total_count >= S.max_num_attempts)); }


        protected:
          const SharedBase& S;
          Writer<> writer;
          const bool always_increment, warn_on_max_attempts;
          std::unique_ptr<File::OFStream> seeds;
          ProgressBar progress;
          EarlyExit early_exit;
      };




      }
    }
  }
}

#endif


