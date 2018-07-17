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

class SDStream : public MethodBase { MEMALIGN(SDStream)
  public:
    class Shared : public SharedBase { MEMALIGN(Shared)
      public:
        Shared (const std::string& diff_path, DWI::Tractography::Properties& property_set) :
            SharedBase (diff_path, property_set),
            lmax (Math::SH::LforN (source.size(3)))
        {
          try {
            Math::SH::check (source);
          } catch (Exception& e) {
            e.display();
            throw Exception ("Algorithm SD_STREAM expects as input a spherical harmonic (SH) image");
          }

          if (is_act() && act().backtrack())
            throw Exception ("Backtracking not valid for deterministic algorithms");

          set_step_size (0.1f);
          dot_threshold = std::cos (max_angle);

          set_cutoff (TCKGEN_DEFAULT_CUTOFF_FOD);

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



    bool init() override
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



    term_t next () override
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


    float get_metric() override
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


