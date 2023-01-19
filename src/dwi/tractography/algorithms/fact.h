/* Copyright (c) 2008-2023 the MRtrix3 contributors.
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

#ifndef __dwi_tractography_algorithms_fact_h__
#define __dwi_tractography_algorithms_fact_h__

#include "interp/masked.h"
#include "interp/nearest.h"

#include "dwi/tractography/tracking/method.h"
#include "dwi/tractography/tracking/shared.h"
#include "dwi/tractography/tracking/tractography.h"
#include "dwi/tractography/tracking/types.h"


namespace MR
{
  namespace DWI
  {
    namespace Tractography
    {
      namespace Algorithms
      {

    using namespace MR::DWI::Tractography::Tracking;

    class FACT : public MethodBase { MEMALIGN(FACT)
      public:
      class Shared : public SharedBase { MEMALIGN(Shared)
        public:
        Shared (const std::string& diff_path, DWI::Tractography::Properties& property_set) :
          SharedBase (diff_path, property_set),
          num_vec (source.size(3)/3) {

          if (source.size(3) % 3)
            throw Exception ("Number of volumes in FACT algorithm input image should be a multiple of 3");

          if (is_act() && act().backtrack())
            throw Exception ("Backtracking not valid for deterministic algorithms");

          if (rk4)
            throw Exception ("4th-order Runge-Kutta integration not valid for FACT algorithm");

          set_step_and_angle (Defaults::stepsize_voxels_firstorder, Defaults::angle_deterministic, false);
          set_num_points();
          set_cutoff (Defaults::cutoff_fixel * (is_act() ? Defaults::cutoff_act_multiplier : 1.0));
          dot_threshold = std::cos (max_angle_1o);

          properties["method"] = "FACT";
        }

        ~Shared () { }

        size_t num_vec;
        float dot_threshold;
      };






      FACT (const Shared& shared) :
        MethodBase (shared),
        S (shared),
        source (S.source) { }

      FACT (const FACT& that) :
        MethodBase (that.S),
        S (that.S),
        source (S.source) { }

      ~FACT () { }



      bool init() override
      {
        if (!get_data (source)) return false;
        if (S.init_dir.allFinite())
          dir = S.init_dir;
        else
          dir = random_direction();
        return select_fixel (dir) >= S.threshold;
      }

      term_t next () override
      {
        if (!get_data (source))
          return EXIT_IMAGE;

        const float max_norm = select_fixel (dir);

        if (max_norm < S.threshold)
          return MODEL;

        pos += S.step_size * dir;
        return CONTINUE;
      }

      float get_metric (const Eigen::Vector3f& position, const Eigen::Vector3f& direction) override
      {
        if (!get_data (source, position))
          return 0.0;
        Eigen::Vector3f d (direction);
        return select_fixel (d);
      }


      protected:
      const Shared& S;
      Interp::Masked<Interp::Nearest<Image<float>>> source;

      float select_fixel (Eigen::Vector3f& d) const
      {

        int idx = -1;
        float max_abs_dot = 0.0, max_dot = 0.0, max_norm = 0.0;

        for (size_t n = 0; n < S.num_vec; ++n) {
          Eigen::Vector3f v (values[3*n], values[3*n+1], values[3*n+2]);
          float norm = v.norm();
          float dot = v.dot(d) / norm;
          float abs_dot = abs (dot);
          if (abs_dot < S.dot_threshold) continue;
          if (max_abs_dot < abs_dot) {
            max_abs_dot = abs_dot;
            max_dot = dot;
            max_norm = norm;
            idx = n;
          }
        }

        if (idx < 0) return (0.0);

        d = { values[3*idx], values[3*idx+1], values[3*idx+2] };
        d.normalize();
        if (max_dot < 0.0)
          d = -d;

        return max_norm;
      }

    };

      }
    }
  }
}

#endif


