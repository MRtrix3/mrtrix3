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


#include <iostream>

#include "dwi/tractography/tracking/generated_track.h"


namespace MR
{
  namespace DWI
  {
    namespace Tractography
    {
      namespace Tracking
      {



        std::ostream& operator<< (std::ostream& stream, GeneratedTrack::status_t status)
        {
          switch (status) {
            case GeneratedTrack::status_t::UNDEFINED:      stream << "Undefined"; break;
            case GeneratedTrack::status_t::SEED_REJECTED:  stream << "Seed rejected"; break;
            case GeneratedTrack::status_t::TRACK_REJECTED: stream << "Track rejected"; break;
            case GeneratedTrack::status_t::ACCEPTED:       stream << "Accepted"; break;
          }
          return stream;
        }



      }
    }
  }
}


