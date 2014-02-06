/*
    Copyright 2011 Brain Research Institute, Melbourne, Australia

    Written by Robert E. Smith and J-Donald Tournier, 2011.

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

#ifndef __dwi_tractography_tracking_write_kernel_h__
#define __dwi_tractography_tracking_write_kernel_h__

#include <ostream>
#include <string>
#include <vector>

#include "ptr.h"
#include "timer.h"

#include "dwi/tractography/file.h"
#include "dwi/tractography/properties.h"
#include "dwi/tractography/streamline.h"

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
      {
        public:

          WriteKernel (const SharedBase& shared,
              const std::string& output_file,
              DWI::Tractography::Properties& properties) :
                S (shared),
                writer (output_file, properties)
          {
            if (properties.find ("seed_output") != properties.end()) {
              seeds = new std::ofstream (properties["seed_output"].c_str(), std::ios_base::trunc);
              (*seeds) << "#Track_index,Seed_index,Pos_x,Pos_y,Pos_z,\n";
            }
          }

          ~WriteKernel ()
          {
            if (App::log_level > 0)
              fprintf (stderr, "\r%8zu generated, %8zu selected    [100%%]\n", writer.total_count, writer.count);
            if (seeds) {
              (*seeds) << "\n";
              seeds->close();
            }

          }


          bool operator() (const GeneratedTrack&);
          bool operator() (const GeneratedTrack&, Tractography::Streamline<>&);

          bool complete() const { return (writer.count >= S.max_num_tracks || writer.total_count >= S.max_num_attempts); }


        private:
          const SharedBase& S;
          Writer<value_type> writer;
          Ptr<std::ofstream> seeds;
          IntervalTimer timer;

      };



      }
    }
  }
}

#endif


