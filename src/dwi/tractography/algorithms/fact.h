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




#ifndef __dwi_tractography_algorithms_fact_h__
#define __dwi_tractography_algorithms_fact_h__

#include "interp/nearest.h"

#include "dwi/tractography/tracking/method.h"
#include "dwi/tractography/tracking/shared.h"
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

    class FACT : public MethodBase {
      public:
      class Shared : public SharedBase {
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

          set_step_size (0.1);
          max_angle *= vox() / step_size;
          dot_threshold = std::cos (max_angle);

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



      bool init()
      {
        if (!get_data (source)) return false;
        if (!S.init_dir.allFinite()) {
          if (!dir.allFinite())
            dir = random_direction();
        } 
        else 
          dir = S.init_dir;
        
        return do_next (dir) >= S.threshold;
      }

      term_t next ()
      {
        if (!get_data (source))
          return EXIT_IMAGE;

        const float max_norm = do_next (dir);

        if (max_norm < S.threshold)
          return BAD_SIGNAL;

        pos += S.step_size * dir;
        return CONTINUE;
      }


      float get_metric()
      {
        Eigen::Vector3f d (dir);
        return do_next (d);
      }


      protected:
      const Shared& S;
      Interp::Nearest<Image<float>> source;

      float do_next (Eigen::Vector3f& d) const
      {

        int idx = -1;
        float max_abs_dot = 0.0, max_dot = 0.0, max_norm = 0.0;

        for (size_t n = 0; n < S.num_vec; ++n) {
          Eigen::Vector3f v (values[3*n], values[3*n+1], values[3*n+2]);
          float norm = v.norm();
          float dot = v.dot(d) / norm; 
          float abs_dot = std::abs (dot);
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


