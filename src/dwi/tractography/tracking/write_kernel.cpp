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

#include "dwi/tractography/tracking/write_kernel.h"


namespace MR
{
  namespace DWI
  {
    namespace Tractography
    {
      namespace Tracking
      {


          bool WriteKernel::operator() (const GeneratedTrack& tck)
          {
            if (complete())
              return false;
            if (tck.size() && output_seeds) {
              const auto& p = tck[tck.get_seed_index()];
              (*output_seeds) << str(writer.count) << "," << str(tck.get_seed_index()) << "," << str(p[0]) << "," << str(p[1]) << "," << str(p[2]) << ",\n";
            }
            switch (tck.get_status()) {
              case GeneratedTrack::status_t::INVALID: assert (0); break;
              case GeneratedTrack::status_t::ACCEPTED: ++selected; ++streamlines; ++seeds; writer (tck); break;
              case GeneratedTrack::status_t::TRACK_REJECTED: ++streamlines; ++seeds; writer.skip(); break;
              case GeneratedTrack::status_t::SEED_REJECTED: ++seeds; break;
            }
            progress.update ([&](){ return printf ("%8" PRIu64 " seeds, %8" PRIu64 " streamlines, %8" PRIu64 " selected", seeds, streamlines, selected); }, always_increment ? true : tck.size());
            if (early_exit (seeds, selected)) {
              WARN ("Track generation terminating prematurely: Highly unlikely to reach target number of streamlines (p<" + str(TCKGEN_EARLY_EXIT_PROB_THRESHOLD,1) + ")");
              return false;
            }
            return true;
          }



      }
    }
  }
}

