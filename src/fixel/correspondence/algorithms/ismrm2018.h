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

#ifndef __fixel_correspondence_algorithms_ismrm2018_h__
#define __fixel_correspondence_algorithms_ismrm2018_h__


#include "header.h"
#include "types.h"
#include "math/math.h"
#include "fixel/correspondence/algorithms/combinatorial.h"


namespace MR {
  namespace Fixel {
    namespace Correspondence {
      namespace Algorithms {




        class ISMRM2018 : public Combinatorial<ISMRM2018>
        {
          public:
            ISMRM2018 (const size_t max_origins_per_target,
                       const size_t max_objectives_per_source,
                       const Header& H_cost) :
                Combinatorial (max_origins_per_target, max_objectives_per_source, H_cost) {}

            FORCE_INLINE static float calculate (const vector<Correspondence::Fixel>& s,
                                                 const vector<Correspondence::Fixel>& rs,
                                                 const vector<Correspondence::Fixel>& t,
                                                 const Eigen::Array<int8_t, Eigen::Dynamic, 1>& objectives_per_source_fixel,
                                                 const Eigen::Array<int8_t, Eigen::Dynamic, 1>& origins_per_remapped_fixel)
            {
              assert (rs.size() == t.size());
              assert (s.size() == size_t(objectives_per_source_fixel.size()));
              float result = 0.0f;
              for (uint32_t index = 0; index != rs.size(); ++index) {
                if (rs[index].density()) {
                  // Differences in fixel orientation contribute in such a way that
                  //   angles of greater than 45 degrees are penalised more severely
                  //   than would be leaving those fixels unmatched
                  result += Math::pow2 (t[index].density() - rs[index].density()) * dp2cost (t[index].absdot(rs[index]));
                } else {
                  result += Math::pow2 (t[index].density());
                }
              }

              // Need to find source fixels that did not contribute to any remapped fixel
              for (uint32_t index = 0; index != s.size(); ++index) {
                if (!objectives_per_source_fixel[index])
                  result += Math::pow2 (s[index].density());
              }

              return result;
            }
        };



      }
    }
  }
}

#endif
