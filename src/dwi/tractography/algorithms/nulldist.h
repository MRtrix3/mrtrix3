/*
   Copyright 2011 Brain Research Institute, Melbourne, Australia

   Written by Robert E. Smith, 2012.

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

#ifndef __dwi_tractography_nulldist_h__
#define __dwi_tractography_nulldist_h__

#include "point.h"
#include "dwi/tractography/method.h"
#include "dwi/tractography/shared.h"



namespace MR
{
  namespace DWI
  {
    namespace Tractography
    {



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

      float get_metric() { return rng.uniform(); }


      protected:
      const Shared& S;
      Interpolator<SourceBufferType::voxel_type>::type source;

      Point<value_type> rand_dir (const Point<value_type>& d) { return (random_direction (d, S.max_angle, S.sin_max_angle)); }

    };

    }
  }
}

#endif


