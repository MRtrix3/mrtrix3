/* Copyright (c) 2008-2017 the MRtrix3 contributors
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


#ifndef __dwi_tractography_tracking_generated_track_h__
#define __dwi_tractography_tracking_generated_track_h__


#include <vector>

#include "dwi/tractography/tracking/types.h"


namespace MR
{
  namespace DWI
  {
    namespace Tractography
    {
      namespace Tracking
      {



        class GeneratedTrack : public vector<Eigen::Vector3f>
        { MEMALIGN(GeneratedTrack)


          public:

            using BaseType = std::vector<Eigen::Vector3f>;

            enum class status_t { INVALID, SEED_REJECTED, TRACK_REJECTED, ACCEPTED };

            GeneratedTrack() : seed_index (0), status (status_t::INVALID) { }
            void clear() { BaseType::clear(); seed_index = 0; status = status_t::INVALID; }
            size_t get_seed_index() const { return seed_index; }
            status_t get_status() const { return status; }
            void reverse() { std::reverse (begin(), end()); seed_index = size()-1; }
            void set_seed_index (const size_t i) { seed_index = i; }
            void set_status (const status_t i) { status = i; }

          private:
            size_t seed_index;
            status_t status;

        };


      }
    }
  }
}

#endif

