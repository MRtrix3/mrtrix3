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


#ifndef __dwi_tractography_algorithms_nulldist_h__
#define __dwi_tractography_algorithms_nulldist_h__

#include "point.h"
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

    class NullDist : public MethodBase {
      public:

      class Shared : public SharedBase {
        public:
        Shared (const std::string& diff_path, DWI::Tractography::Properties& property_set) :
          SharedBase (diff_path, property_set)
        {
          set_step_size (0.1);
          sin_max_angle = Math::sin (max_angle);
          properties["method"] = "Nulldist";
        }
        value_type sin_max_angle;
      };

      NullDist (const Shared& shared) :
        MethodBase (shared),
        S (shared),
        source (S.source_voxel) { }


      bool init () {
        if (!get_data (source))
          return false;
        if (!S.init_dir) {
          dir.set (rng.normal(), rng.normal(), rng.normal());
          dir.normalise();
        } else {
          dir = S.init_dir;
        }
        return true;
      }

      term_t next () {
        if (!get_data (source))
          return EXIT_IMAGE;
        dir = rand_dir (dir);
        dir.normalise();
        pos += S.step_size * dir;
        return CONTINUE;
      }


      protected:
      const Shared& S;
      Interpolator<SourceBufferType::voxel_type>::type source;

      Point<value_type> rand_dir (const Point<value_type>& d) { return (random_direction (d, S.max_angle, S.sin_max_angle)); }

    };

      }
    }
  }
}

#endif


