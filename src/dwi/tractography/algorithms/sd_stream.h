/*******************************************************************************
    Copyright (C) 2014 Brain Research Institute, Melbourne, Australia
    
    Permission is hereby granted under the Patent Licence Agreement between
    the BRI and Siemens AG from July 3rd, 2012, to Siemens AG obtaining a
    copy of this software and associated documentation files (the
    "Software"), to deal in the Software without restriction, including
    without limitation the rights to possess, use, develop, manufacture,
    import, offer for sale, market, sell, lease or otherwise distribute
    Products, and to permit persons to whom the Software is furnished to do
    so, subject to the following conditions:
    
    The above copyright notice and this permission notice shall be included
    in all copies or substantial portions of the Software.
    
    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
    OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
    MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
    IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
    CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
    TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
    SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

*******************************************************************************/


#ifndef __dwi_tractography_algorithms_sd_stream_h__
#define __dwi_tractography_algorithms_sd_stream_h__

#include "point.h"
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
            lmax (Math::SH::LforN (source_buffer.dim(3)))
        {

          set_step_size (0.1);
          dot_threshold = Math::cos (max_angle);

          if (rk4) {
            INFO ("minimum radius of curvature = " + str(step_size / (max_angle / (0.5 * M_PI))) + " mm");
          } else {
            INFO ("minimum radius of curvature = " + str(step_size / ( 2.0 * sin (max_angle / 2.0))) + " mm");
          }

          properties["method"] = "SDStream";

          bool precomputed = true;
          properties.set (precomputed, "sh_precomputed");
          if (precomputed)
            precomputer = new Math::SH::PrecomputedAL<value_type> (lmax);
        }

        ~Shared () {
          if (precomputer)
            delete precomputer;
        }

        value_type dot_threshold;
        size_t lmax;
        Math::SH::PrecomputedAL<value_type>* precomputer;

    };






    SDStream (const Shared& shared) :
      MethodBase (shared),
      S (shared),
      source (S.source_voxel) { }

    SDStream (const SDStream& that) :
      MethodBase (that.S),
      S (that.S),
      source (S.source_voxel) { }


    ~SDStream () { }



    bool init ()
    {
      if (!get_data (source))
        return (false);

      if (!S.init_dir) {
        if (!dir.valid())
          dir.set (rng.normal(), rng.normal(), rng.normal());
      } else {
        dir = S.init_dir;
      }

      dir.normalise();
      if (!find_peak())
        return false;

      return true;
    }



    term_t next ()
    {
      if (!get_data (source))
        return EXIT_IMAGE;

      const Point<value_type> prev_dir (dir);

      if (!find_peak())
        return BAD_SIGNAL;

      if (prev_dir.dot (dir) < S.dot_threshold)
        return HIGH_CURVATURE;

      pos += dir * S.step_size;
      return CONTINUE;
    }



    protected:
      const Shared& S;
      Interpolator<SourceBufferType::voxel_type>::type source;

      value_type find_peak ()
      {
        value_type FOD = Math::SH::get_peak (&values[0], S.lmax, dir, S.precomputer);
        if (!std::isfinite (FOD) || FOD < S.threshold)
          FOD = 0.0;
        return FOD;
      }

      value_type FOD (const Point<value_type>& d) const
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


