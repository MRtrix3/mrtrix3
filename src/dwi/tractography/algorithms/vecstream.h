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




#ifndef __dwi_tractography_algorithms_vecstream_h__
#define __dwi_tractography_algorithms_vecstream_h__

#include "point.h"
#include "image/interp/nearest.h"

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

    class VecStream : public MethodBase {
      public:
      class Shared : public SharedBase {
        public:
        Shared (const std::string& diff_path, DWI::Tractography::Properties& property_set) :
          SharedBase (diff_path, property_set),
          num_vec (source_buffer.dim(3)/3) {

          if (is_act() && act().backtrack())
            throw Exception ("Backtracking not valid for deterministic algorithms");

          if (rk4)
            throw Exception ("4th-order Runge-Kutta integration not valid for VecStream algorithm");

          set_step_size (0.1);
          max_angle *= vox() / step_size;
          dot_threshold = Math::cos (max_angle);

          properties["method"] = "VecStream";
        }

        ~Shared () { }

        size_t num_vec;
        value_type dot_threshold;
      };






      VecStream (const Shared& shared) :
        MethodBase (shared),
        S (shared),
        source (S.source_voxel) { }

      VecStream (const VecStream& that) :
        MethodBase (that.S),
        S (that.S),
        source (S.source_voxel) { }

      ~VecStream () { }



      bool init (bool keep_sampling)
      {
        if (!get_data (source)) return false;
        if (!S.init_dir) {
          if (!dir.valid())
            dir.set (rng.normal(), rng.normal(), rng.normal());
        } else {
          dir = S.init_dir;
        }
        return do_next (dir) >= S.threshold;
      }

      term_t next ()
      {
        if (!get_data (source))
          return EXIT_IMAGE;

        const value_type max_norm = do_next (dir);

        if (max_norm < S.threshold)
          return BAD_SIGNAL;

        pos += S.step_size * dir;
        return CONTINUE;
      }


      float get_metric()
      {
        Point<value_type> d (dir);
        return do_next (d);
      }


      protected:
      const Shared& S;
      Image::Interp::Nearest<SourceBufferType::voxel_type> source;

      // This function is now identical to the original vecstream
      value_type do_next (Point<value_type>& d) const
      {

        int idx = -1;
        value_type max_abs_dot = 0.0, max_dot = 0.0, max_norm = 0.0;

        for (size_t n = 0; n < S.num_vec; ++n) {
          const value_type* const m = &values[3*n];
          value_type norm = Math::norm(m,3);
          value_type dot = Math::dot (m, (value_type*) d, 3) / norm;
          value_type abs_dot = Math::abs (dot);
          if (abs_dot < S.dot_threshold) continue;
          if (max_abs_dot < abs_dot) {
            max_abs_dot = abs_dot;
            max_dot = dot;
            max_norm = norm;
            idx = n;
          }
        }

        if (idx < 0) return (0.0);

        d.set (values[3*idx], values[3*idx+1], values[3*idx+2]);
        d.normalise();
        if (max_dot < 0.0) d = -d;

        return (max_norm);
      }

    };

      }
    }
  }
}

#endif


