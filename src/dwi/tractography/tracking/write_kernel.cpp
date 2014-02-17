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
            if (tck.size() && seeds) {
              const Point<float>& p = tck[tck.get_seed_index()];
              (*seeds) << str(writer.count) << "," << str(tck.get_seed_index()) << "," << str(p[0]) << "," << str(p[1]) << "," << str(p[2]) << ",\n";
            }
            writer (tck);
            if (timer && App::log_level > 0) {
              fprintf (stderr, "\r%8zu generated, %8zu selected    [%3d%%]",
                  writer.total_count, writer.count,
                  (int(100.0 * std::max (writer.total_count/float(S.max_num_attempts), writer.count/float(S.max_num_tracks)))));
            }
            return true;
          }

          bool WriteKernel::operator() (const GeneratedTrack& in, Tractography::Streamline<>& out)
          {
            out.index = writer.count;
            out.weight = 1.0;
            if (!operator() (in))
              return false;
            out = in;
            return true;
          }




      }
    }
  }
}

