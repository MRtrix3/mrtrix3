/*
    Copyright 2009 Brain Research Institute, Melbourne, Australia

    Written by J-Donald Tournier, 25/10/09.

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

#ifndef __dwi_tractography_method_h__
#define __dwi_tractography_method_h__

#include "point.h"
#include "math/rng.h"
#include "image/voxel.h"
#include "dataset/interp/linear.h"
#include "dwi/tractography/shared.h"

namespace MR {
  namespace DWI {
    namespace Tractography {

      class MethodBase {
        public:
          MethodBase (const SharedBase& shared) : 
            source (shared.source), interp (source), rng (rng_seed++), values (new float [source.dim(3)]) { }

          MethodBase (const MethodBase& base) : 
            source (base.source), interp (source), rng (rng_seed++), values (new float [source.dim(3)]) { }

          ~MethodBase () { delete values; }

          StorageType source;
          DataSet::Interp::Linear<StorageType> interp;
          Math::RNG rng;
          Point pos, dir;
          float* values;

          bool get_data (const Point& position) 
          {
            interp.scanner (position); 
            if (!interp) return (false);
            for (source[3] = 0; source[3] < source.dim(3); ++source[3]) 
              values[source[3]] = interp.value();
            return (!isnan (values[0]));
          }

          bool get_data () { return (get_data (pos)); }

          Point random_direction (const Point& d, float max_angle, float sin_max_angle);

          static void init () { rng_seed = time (NULL); }

        private:
          static size_t rng_seed;
      };



    }
  }
}

#endif



