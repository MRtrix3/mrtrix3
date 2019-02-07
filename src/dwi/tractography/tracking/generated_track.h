/* Copyright (c) 2008-2019 the MRtrix3 contributors.
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

#ifndef __dwi_tractography_tracking_generated_track_h__
#define __dwi_tractography_tracking_generated_track_h__


#include "types.h"

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

            using BaseType = vector<Eigen::Vector3f>;

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

