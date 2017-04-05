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



    class GeneratedTrack : public std::vector<Eigen::Vector3f>
    {

        using BaseType = std::vector<Eigen::Vector3f>;

      public:
        GeneratedTrack() : seed_index (0) { }
        void clear() { BaseType::clear(); seed_index = 0; }
        size_t get_seed_index() const { return seed_index; }
        void reverse() { std::reverse (begin(), end()); seed_index = size()-1; }
        void set_seed_index (const size_t i) { seed_index = i; }

      private:
        size_t seed_index;

    };


      }
    }
  }
}

#endif

