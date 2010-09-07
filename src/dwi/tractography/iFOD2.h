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

#ifndef __dwi_tractography_iFOD2_h__
#define __dwi_tractography_iFOD2_h__

#include <algorithm>

#include "point.h"
#include "math/SH.h"
#include "dwi/tractography/method.h"
#include "dwi/tractography/shared.h"
#include "dwi/tractography/direction_grid.h"

#define NUM_CALIBRATE 1000

namespace MR {
  namespace DWI {
    namespace Tractography {

      class iFOD2 : public MethodBase {
        public:
          class Shared : public SharedBase {
            public:
              Shared (Image::Header& source, DWI::Tractography::Properties& property_set) :
                SharedBase (source, property_set),
                lmax (Math::SH::LforN (source.dim(3))), 
                num_samples (1),
                max_trials (100),
                sin_max_angle (sin (max_angle)) {
                  properties["method"] = "iFOD2";
                  properties.set (lmax, "lmax");
                  properties.set (num_samples, "samples_per_step");
                  properties.set (max_trials, "max_trials");
                  bool precomputed = true;
                  properties.set (precomputed, "sh_precomputed");
                  if (precomputed) precomputer.init (lmax);
                  info ("minimum radius of curvature = " + str(step_size / max_angle) + " mm");
                }

              size_t lmax, num_samples, max_trials;
              value_type sin_max_angle;
              Math::SH::PrecomputedAL<value_type> precomputer;
          };

          iFOD2 (const Shared& shared) :
            MethodBase (shared), 
            S (shared), 
            mean_sample_num (0), 
            num_sample_runs (0) 
          { 
            using namespace Math;

            Vector<value_type> fod (values, source.dim(3));
            Point<value_type> positions [S.num_samples], tangents [S.num_samples];

            pos.set (0.0, 0.0, 0.0);
            dir.set (0.0, 0.0, 1.0);

            SH::delta (fod, dir, S.lmax);
            value_type peak = pow (SH::value (values, dir, S.lmax), S.num_samples);

            {
              ProgressBar progress ("calibrating rejection sampling...");

              for (size_t extent = 1; extent < 9; ++extent) {
                value_type min = std::numeric_limits<value_type>::infinity();
                calibrate_list = direction_grid (S.max_angle, extent);

                for (size_t n = 0; n < NUM_CALIBRATE; ++n) {
                  value_type max = 0.0;
                  Point<value_type> end_dir = rand_dir (dir);

                  for (size_t i = 0; i < calibrate_list.size(); ++i) {
                    get_path (positions, tangents, calibrate_list[i]);

                    value_type prob = 1.0;
                    for (size_t i = 0; i < S.num_samples; ++i) {
                      SH::delta (fod, get_tangent (positions[i], end_dir, S.step_size), S.lmax);
                      prob *= SH::value (values, tangents[i], S.lmax);
                    }

                    if (prob > max) 
                      max = prob;

                    ++progress;
                  }

                  if (max < min)
                    min = max;
                }

                calibrate_ratio = 1.1 * peak / min;
                if (calibrate_ratio < 3.0) 
                  break;
              }
            }

            info ("rejection sampling will use " + str (calibrate_list.size()) 
                + " directions with a ratio of " + str (calibrate_ratio));
          } 



          ~iFOD2 () 
          { 
            info ("mean number of samples per step = " + str (value_type(mean_sample_num)/value_type(num_sample_runs))); 
          }

          bool init () 
          { 
            if (!get_data ()) return (false);

            if (!S.init_dir) {
              for (size_t n = 0; n < S.max_trials; n++) {
                dir.set (rng.normal(), rng.normal(), rng.normal());
                dir.normalise();
                value_type val = FOD (dir);
                if (!isnan (val)) {
                  if (val > S.init_threshold) 
                    return (true);
                }
              }   
            }   
            else {
              dir = S.init_dir;
              value_type val = FOD (dir);
              if (finite (val)) { 
                if (val > S.init_threshold) 
                  return (true);
              }
            }   

            return (false);
          }   

          bool next () 
          {
            Point<value_type> next_pos, next_dir;
            Point<value_type> positions [S.num_samples], tangents [S.num_samples];

            value_type max_val = 0.0;
            for (size_t i = 0; i < calibrate_list.size(); ++i) {
              get_path (positions, tangents, rotate_direction (dir, calibrate_list[i]));
              value_type val = path_prob (positions, tangents);
              if (val > max_val) 
                max_val = val;
            }

            if (max_val <= 0.0 || !finite (max_val))
              return (false);

            max_val *= calibrate_ratio;

            for (size_t n = 0; n < S.max_trials; n++) {
              value_type val = rand_path_prob (next_pos, next_dir);

              if (val > max_val) 
                info ("max_val exceeded!!! (val = " + str(val) + ", max_val = " + str (max_val) + ")");

              if (rng.uniform() < val/max_val) {
                dir = next_dir;
                dir.normalise();
                pos = next_pos;
                mean_sample_num += n;
                num_sample_runs++;
                return (true);
              }
            }

            return (false);
          }

        private:
          const Shared& S;
          value_type calibrate_ratio;
          size_t mean_sample_num, num_sample_runs;
          std::vector<Point<value_type> > calibrate_list;

          value_type FOD (const Point<value_type>& direction) const 
          {
            return (S.precomputer ?  
                S.precomputer.value (values, direction) : 
                Math::SH::value (values, direction, S.lmax)
                );
          }

          value_type FOD (const Point<value_type>& position, const Point<value_type>& direction) 
          {
            if (!get_data (position)) 
              return (NAN);
            return (FOD (direction));
          }

          value_type rand_path_prob (Point<value_type>& next_pos, Point<value_type>& next_dir) 
          {
            Point<value_type> positions [S.num_samples], tangents [S.num_samples];
            get_path (positions, tangents, rand_dir (dir));

            value_type prob = path_prob (positions, tangents);

            next_pos = positions[S.num_samples-1];
            next_dir = tangents[S.num_samples-1];

            return (prob);
          }


          value_type path_prob (Point<value_type>* positions, Point<value_type>* tangents)
          {
            value_type prob = 1.0;
            for (size_t i = 0; i < S.num_samples; ++i) {
              value_type fod_amp = FOD (positions[i], tangents[i]);
              if (isnan (fod_amp) || fod_amp < S.threshold) 
                return (NAN);
              prob *= fod_amp;
            }

            return (prob);
          }





          void get_path (Point<value_type>* positions, Point<value_type>* tangents, const Point<value_type>& end_dir) 
          {
            value_type cos_theta = end_dir.dot (dir);
            cos_theta = std::min (cos_theta, value_type(1.0));
            value_type theta = Math::acos (cos_theta);

            if (theta) {
              Point<value_type> curv = end_dir - cos_theta * dir;
              curv.normalise();
              value_type R = S.step_size / theta;

              for (size_t i = 0; i < S.num_samples-1; ++i) {
                value_type a = (theta * (i+1)) / S.num_samples;
                value_type cos_a = Math::cos (a);
                value_type sin_a = Math::sin (a);
                *positions++ = pos + R * (sin_a * dir + (value_type(1.0) - cos_a) * curv);
                *tangents++ = cos_a * dir + sin_a * curv;
              }
              *positions = pos + R * (Math::sin (theta) * dir + (value_type(1.0)-cos_theta) * curv);
              *tangents = end_dir;
            }
            else { // straight on:
              for (size_t i = 0; i <= S.num_samples; ++i) {
                value_type f = (i+1) * (S.step_size / S.num_samples);
                *positions++ = pos + f * dir;
                *tangents++ = dir;
              }
            }
          }

          Point<value_type> rand_dir (const Point<value_type>& d) { return (random_direction (d, S.max_angle, S.sin_max_angle)); }
      };

    }
  }
}

#endif

