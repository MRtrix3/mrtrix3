/*
    Copyright 2011 Brain Research Institute, Melbourne, Australia

    Written by J-Donald Tournier and Robert E. Smith, 2011.

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

        typedef std::vector<Eigen::Vector3f> BaseType;

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

