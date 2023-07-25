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

#ifndef __fixel_correspondence_algorithms_in2023_h__
#define __fixel_correspondence_algorithms_in2023_h__

#include <stdint.h>

#include "header.h"

#include "fixel/correspondence/algorithms/combinatorial.h"


namespace MR {

  namespace App {
    class OptionGroup;
  }

  namespace Fixel {
    namespace Correspondence {
      namespace Algorithms {



        extern App::OptionGroup IN2023Options;



        class IN2023 : public Combinatorial<IN2023>
        {
          public:

            IN2023 (const size_t max_origins_per_target,
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
              for (uint32_t t_index = 0; t_index != rs.size(); ++t_index) {

                result += t[t_index].density() * (rs[t_index].density() ?
                                                  dp2cost (t[t_index].absdot(rs[t_index])) :
                                                  1.0f);

                result += a * Math::pow2 (t[t_index].density() - rs[t_index].density());

                result += b * Math::pow2 (origins_per_remapped_fixel[t_index] - int8_t(1));

              }

              for (uint32_t s_index = 0; s_index != s.size(); ++s_index) {

                if (!objectives_per_source_fixel[s_index]) {
                  result += s[s_index].density();
                  result += a * Math::pow2 (s[s_index].density());
                }

                result += b * Math::pow2 (objectives_per_source_fixel[s_index] - int8_t(1));

              }

              return result;
            }

            static void set_constants (const float alpha, const float beta) { a = alpha; b = beta; }

          protected:
            static float a, b;
        };



      }
    }
  }
}

#endif
