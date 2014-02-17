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


