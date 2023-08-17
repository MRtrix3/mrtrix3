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

#ifndef __fixel_correspondence_algorithms_nearest_h__
#define __fixel_correspondence_algorithms_nearest_h__


#include "types.h"

#include "fixel/correspondence/algorithms/base.h"


namespace MR {

  namespace App {
    class OptionGroup;
  }

  namespace Fixel {
    namespace Correspondence {
      namespace Algorithms {



        extern App::OptionGroup NearestOptions;



        // Duplicate the functionality of the old fixelcorrespondence command:
        // For each target fixel, simply select the closest source fixel,
        //   as long as it is within some angular limit
        class Nearest : public Base
        {
          public:
            Nearest (const float max_angle) :
                dp_threshold (std::cos (max_angle*Math::pi/180.0)) {}
            virtual ~Nearest() {}

            vector< vector<uint32_t> > operator() (const voxel_t&,
                                                   const vector<Correspondence::Fixel>& s,
                                                   const vector<Correspondence::Fixel>& t) const final
            {
              vector< vector<uint32_t> > result;
              for (uint32_t it = 0; it != t.size(); ++it) {
                uint32_t closest_index = 0;
                float max_dp = 0.0f;
                for (uint32_t is = 0; is != s.size(); ++is) {
                  const float dp = t[it].absdot (s[is]);
                  if (dp > max_dp) {
                    max_dp = dp;
                    closest_index = is;
                  }
                }
                if (max_dp > dp_threshold) {
                  result.emplace_back (vector<uint32_t> (1, closest_index));
                } else {
                  result.emplace_back (vector<uint32_t>());
                }
              }
              return result;
            }

          protected:
            const float dp_threshold;
        };




      }
    }
  }
}

#endif
