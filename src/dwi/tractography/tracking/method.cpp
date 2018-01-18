/*
 * Copyright (c) 2008-2018 the MRtrix3 contributors.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, you can obtain one at http://mozilla.org/MPL/2.0/
 *
 * MRtrix3 is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * For more details, see http://www.mrtrix.org/
 */


#include "dwi/tractography/tracking/method.h"



namespace MR
{
  namespace DWI
  {
    namespace Tractography
    {
      namespace Tracking
      {



        void MethodBase::truncate_track (GeneratedTrack& tck, const size_t length_to_revert_from, const size_t revert_step)
        {
          if (tck.get_seed_index() + revert_step >= length_to_revert_from) {
            tck.clear();
            pos = { NaN, NaN, NaN };
            dir = { NaN, NaN, NaN };
            return;
          }
          const size_t new_size = length_to_revert_from - revert_step;
          if (tck.size() == 2 || new_size == 1)
            dir = (tck[1] - tck[0]).normalized();
          else
            dir = (tck[new_size] - tck[new_size - 2]).normalized();
          tck.resize (length_to_revert_from - revert_step);
          pos = tck.back();
          if (act_method_additions)
            act().sgm_depth = (act().sgm_depth > revert_step) ? act().sgm_depth - revert_step : 0;
        }



        bool MethodBase::check_seed()
        {
          if (!pos.allFinite())
            return false;

          if ((S.properties.mask.size() && !S.properties.mask.contains (pos))
              || (S.properties.exclude.contains (pos))
              || (S.is_act() && !act().check_seed (pos))) {
            pos = { NaN, NaN, NaN };
            return false;
          }

          return true;
        }



        Eigen::Vector3f MethodBase::random_direction ()
        {
          Eigen::Vector3f d;
          do {
            d[0] = 2.0 * uniform(*rng) - 1.0;
            d[1] = 2.0 * uniform(*rng) - 1.0;
            d[2] = 2.0 * uniform(*rng) - 1.0;
          } while (d.squaredNorm() > 1.0);
          d.normalize();
          return d;
        }



        Eigen::Vector3f MethodBase::random_direction (const float max_angle, const float sin_max_angle)
        {
          float phi = 2.0 * Math::pi * uniform(*rng);
          float theta;
          do {
            theta = max_angle * uniform(*rng);
          } while (sin_max_angle * uniform(*rng) > sin (theta));
          return Eigen::Vector3f (sin(theta)*cos(phi), sin(theta)*sin(phi), cos(theta));
        }



        Eigen::Vector3f MethodBase::rotate_direction (const Eigen::Vector3f& reference, const Eigen::Vector3f& direction)
        {
          float n = std::sqrt (Math::pow2(reference[0]) + Math::pow2(reference[1]));
          if (n == 0.0) {
            if (reference[2] < 0.0)
              return -direction;
            else
              return direction;
          }

          Eigen::Vector3f m (reference[0]/n, reference[1]/n, 0.0f);
          Eigen::Vector3f mp (reference[2]*m[0], reference[2]*m[1], -n);

          float alpha = direction[2];
          float beta = direction[0]*m[0] + direction[1]*m[1];

          return {
              direction[0] + alpha * reference[0] + beta * (mp[0] - m[0]),
              direction[1] + alpha * reference[1] + beta * (mp[1] - m[1]),
              direction[2] + alpha * (reference[2]-1.0f) + beta * (mp[2] - m[2])
          };
        }



      }
    }
  }
}


