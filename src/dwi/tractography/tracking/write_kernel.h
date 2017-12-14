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


#ifndef __dwi_tractography_tracking_write_kernel_h__
#define __dwi_tractography_tracking_write_kernel_h__

#include <cinttypes>
#include <string>

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
                warn_on_max_seeds (S.implicit_max_num_seeds),
                seeds (0),
                streamlines (0),
                selected (0),
                progress (printf ("       0 seeds,        0 streamlines,        0 selected", 0, 0), always_increment ? S.max_num_seeds : S.max_num_tracks),
                early_exit (shared)
          {
            const auto p = properties.find ("seed_output");
            if (p != properties.end()) {
              output_seeds.reset (new File::OFStream (p->second, std::ios_base::out | std::ios_base::trunc));
              (*output_seeds) << "#Track_index,Seed_index,Pos_x,Pos_y,Pos_z,\n";
            }
          }

          WriteKernel (const WriteKernel&) = delete;
          WriteKernel& operator= (const WriteKernel&) = delete;

          ~WriteKernel ()
          {
            // Use set_text() rather than update() here to force update of the text before progress goes out of scope
            progress.set_text (printf ("%8" PRIu64 " seeds, %8" PRIu64 " streamlines, %8" PRIu64 " selected", seeds, streamlines, selected));
            if (warn_on_max_seeds && writer.total_count == S.max_num_seeds
                && S.max_num_tracks && writer.count < S.max_num_tracks) {
              WARN ("less than desired streamline number due to implicit maximum number of seeds; set -seeds 0 to override");
            }
            if (output_seeds) {
              (*output_seeds) << "\n";
              output_seeds->close();
            }

          }


          bool operator() (const GeneratedTrack&);

          bool complete() const { return ((S.max_num_tracks && selected >= S.max_num_tracks) || (S.max_num_seeds && seeds >= S.max_num_seeds)); }


        protected:
          const SharedBase& S;
          Writer<> writer;
          const bool always_increment, warn_on_max_seeds;
          size_t seeds, streamlines, selected;
          std::unique_ptr<File::OFStream> output_seeds;
          ProgressBar progress;
          EarlyExit early_exit;
      };




      }
    }
  }
}

#endif


