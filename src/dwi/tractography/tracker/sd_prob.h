/*
    Copyright 2008 Brain Research Institute, Melbourne, Australia

    Written by J-Donald Tournier, 27/06/08.

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

#ifndef __dwi_tractography_tracker_sd_prob_h__
#define __dwi_tractography_tracker_sd_prob_h__

#include "math/SH.h"
#include "dwi/tractography/tracker/base.h"

namespace MR {
  namespace DWI {
    namespace Tractography {
      namespace Tracker {

        class SDProb : public Base {
          public:
            SDProb (Image::Object& source_image, Properties& properties);

          protected:
            float min_dpi, dist_spread;
            int   lmax, max_trials;
            bool  precomputed;
            Math::SH::PrecomputedAL<float>*  precomputer;

            virtual bool  init_direction (const Point& seed_dir);
            virtual bool  next_point ();

            Point         new_rand_dir ();
        };





        inline Point SDProb::new_rand_dir ()
        {
          float v[3];
          do { 
            v[0] = 2.0*rng.uniform() - 1.0; 
            v[1] = 2.0*rng.uniform() - 1.0; 
          } while (v[0]*v[0] + v[1]*v[1] > 1.0); 

          v[0] *= dist_spread;
          v[1] *= dist_spread;
          v[2] = 1.0 - (v[0]*v[0] + v[1]*v[1]);
          v[2] = v[2] < 0.0 ? 0.0 : sqrt (v[2]);

          if (dir[0]*dir[0] + dir[1]*dir[1] < 1e-4) 
            return (Point (v[0], v[1], dir[2] > 0.0 ? v[2] : -v[2]));

          float y[] = { dir[0], dir[1], 0.0 };
          Math::normalise (y);
          float x[] =  { -y[1], y[0], 0.0 };
          float y2[] = { -x[1]*dir[2], x[0]*dir[2], x[1]*dir[0] - x[0]*dir[1] };
          Math::normalise (y2);

          float cx = v[0]*x[0] + v[1]*x[1];
          float cy = v[0]*y[0] + v[1]*y[1];

          return (Point (
                cx*x[0] + cy*y2[0] + v[2]*dir[0], 
                cx*x[1] + cy*y2[1] + v[2]*dir[1],
                cy*y2[2] + v[2]*dir[2]) );
        }

      }
    }
  }
}

#endif

