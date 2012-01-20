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

#ifndef __dwi_tractography_vecstream_h__
#define __dwi_tractography_vecstream_h__

#include "point.h"
#include "dwi/tractography/method.h"
#include "dwi/tractography/shared.h"
#include "image/interp/nearest.h"


namespace MR {
  namespace DWI {
    namespace Tractography {


      class VecStream : public MethodBase {
        public:
          class Shared : public SharedBase {
            public:
              Shared (Image::Header& source, DWI::Tractography::Properties& property_set) :
                SharedBase (source, property_set),
                num_vec (source.dim(3)/3) {

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
            ninterp (source) { }

          VecStream (const VecStream& vec) : 
            MethodBase (vec.S), 
            S (vec.S),
            ninterp (source) { }


          ~VecStream () { }



          bool init ()
          { 
            if (!S.init_dir) 
              dir.set (rng.normal(), rng.normal(), rng.normal());
            else 
              dir = S.init_dir;

            return get_data (dir) >= S.threshold;
          }   



          bool next () 
          {
            if (get_data (dir) < S.threshold)
              return false;

            pos += S.step_size * dir;
            return true;
          }

        protected:
          const Shared& S;
          Image::Interp::Nearest<VoxelType> ninterp;

          value_type get_data (Point<value_type>& d) 
          {
            ninterp.scanner (pos); 
            if (!ninterp) return (false);
            for (source[3] = 0; source[3] < source.dim(3); ++source[3]) 
              values[source[3]] = ninterp.value();

            int idx = -1;
            value_type max_abs_dot = 0.0, max_dot = 0.0, max_norm = 0.0;
            
            for (size_t n = 0; n < S.num_vec; ++n) {
              value_type* m = values + 3*n;
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

#endif


