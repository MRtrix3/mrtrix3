/*
   Copyright 2009 Brain Research Institute, Melbourne, Australia

   Written by Robert E. Smith, 23/01/2012.

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

#ifndef __dwi_tractography_algorithms_sd_stream_h__
#define __dwi_tractography_algorithms_sd_stream_h__

#include "math/SH.h"
#include "dwi/tractography/tracking/method.h"
#include "dwi/tractography/tracking/shared.h"
#include "dwi/tractography/tracking/types.h"


namespace MR {
namespace DWI {
namespace Tractography {
namespace Algorithms {

using namespace MR::DWI::Tractography::Tracking;

class SDStream : public MethodBase {
  public:
    class Shared : public SharedBase {
      public:
        Shared (const std::string& diff_path, DWI::Tractography::Properties& property_set) :
            SharedBase (diff_path, property_set),
            lmax (Math::SH::LforN (source.size(3)))
        {
          if (source.size(3) != int (Math::SH::NforL (Math::SH::LforN (source.size(3))))) 
            throw Exception ("number of volumes in input data does not match that expected for a SH dataset");

          if (is_act() && act().backtrack())
            throw Exception ("Backtracking not valid for deterministic algorithms");

          set_step_size (0.1);
          dot_threshold = std::cos (max_angle);

          if (rk4) {
            INFO ("minimum radius of curvature = " + str(step_size / (max_angle / (0.5 * Math::pi))) + " mm");
          } else {
            INFO ("minimum radius of curvature = " + str(step_size / ( 2.0 * sin (max_angle / 2.0))) + " mm");
          }

          properties["method"] = "SDStream";

          bool precomputed = true;
          properties.set (precomputed, "sh_precomputed");
          if (precomputed)
            precomputer = new Math::SH::PrecomputedAL<float> (lmax);
        }

        ~Shared () {
          if (precomputer)
            delete precomputer;
        }

        float dot_threshold;
        size_t lmax;
        Math::SH::PrecomputedAL<float>* precomputer;

    };






    SDStream (const Shared& shared) :
      MethodBase (shared),
      S (shared),
      source (S.source) { }

    SDStream (const SDStream& that) :
      MethodBase (that.S),
      S (that.S),
      source (S.source) { }


    ~SDStream () { }



    bool init()
    {
      if (!get_data (source))
        return (false);

      if (!S.init_dir.allFinite()) {
        if (!dir.allFinite())
          dir = random_direction();
      } 
      else 
        dir = S.init_dir;

      dir.normalize();
      if (!find_peak())
        return false;

      return true;
    }



    term_t next ()
    {
      if (!get_data (source))
        return EXIT_IMAGE;

      const Eigen::Vector3f prev_dir (dir);

      if (!find_peak())
        return BAD_SIGNAL;

      if (prev_dir.dot (dir) < S.dot_threshold)
        return HIGH_CURVATURE;

      pos += dir * S.step_size;
      return CONTINUE;
    }


    float get_metric()
    {
      return FOD (dir);
    }


    protected:
      const Shared& S;
      Interpolator<Image<float>>::type source;

      float find_peak ()
      {
        float FOD = Math::SH::get_peak (values, S.lmax, dir, S.precomputer);
        if (!std::isfinite (FOD) || FOD < S.threshold)
          FOD = 0.0;
        return FOD;
      }

      float FOD (const Eigen::Vector3f& d) const
      {
        return (S.precomputer ?
            S.precomputer->value (values, d) :
            Math::SH::value (values, d, S.lmax)
        );
      }


};

}
}
}
}

#endif


