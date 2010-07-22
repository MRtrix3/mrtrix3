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

#include "point.h"
#include "math/SH.h"
#include "dwi/tractography/method.h"
#include "dwi/tractography/shared.h"


namespace MR {
  namespace DWI {
    namespace Tractography {

      class iFOD2 : public MethodBase {
        public:
          class Shared : public SharedBase {
            public:
              Shared (const Image::Header& source, DWI::Tractography::Properties& property_set) :
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
                  prob_threshold = Math::pow (threshold, num_samples);
                  info ("minimum radius of curvature = " + str(step_size / max_angle) + " mm");
                }

              size_t lmax, num_samples, max_trials;
              value_type sin_max_angle, prob_threshold;
              Math::SH::PrecomputedAL<value_type> precomputer;
          };

          iFOD2 (const Shared& shared) :
            MethodBase (shared), 
            S (shared), 
            mean_sample_num (0), 
            num_sample_runs (0) { } 

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
                  if (val > S.init_threshold) {
                    prev_prob_val = Math::pow (val, S.num_samples);
                    return (true);
                  }
                }
              }   
            }   
            else {
              dir = S.init_dir;
              value_type val = FOD (dir);
              if (finite (val)) { 
                if (val > S.init_threshold) {
                  prev_prob_val = Math::pow (val, S.num_samples);
                  return (true);
                }
              }
            }   

            return (false);
          }   

          bool next () 
          {
            Point<value_type> next_pos, next_dir;

            value_type max_val_actual = 0.0;
            for (int n = 0; n < 100; n++) {
              value_type val = rand_path (next_pos, next_dir);
              if (val > max_val_actual) max_val_actual = val;
            }
            value_type max_val = MAX (prev_prob_val, max_val_actual);
            prev_prob_val = max_val_actual;

            if (isnan (max_val) || max_val < S.prob_threshold) return (false);
            max_val *= 1.5;

            size_t nmax = max_val_actual > S.prob_threshold ? 10000 : S.max_trials;
            for (size_t n = 0; n < nmax; n++) {
              value_type val = rand_path (next_pos, next_dir);
              if (val > S.prob_threshold) {
                if (val > max_val) info ("max_val exceeded!!! (val = " + str(val) + ", max_val = " + str (max_val) + ")");
                if (rng.uniform() < val/max_val) {
                  dir = next_dir;
                  dir.normalise();
                  pos = next_pos;
                  mean_sample_num += n;
                  num_sample_runs++;
                  return (true);
                }
              }
            }

            return (false);
          }

        private:
          const Shared& S;
          value_type prev_prob_val;
          size_t mean_sample_num, num_sample_runs;

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

          value_type rand_path (Point<value_type>& next_pos, Point<value_type>& next_dir) 
          {
            next_dir = rand_dir (dir);
            value_type cos_theta = next_dir.dot (dir);
            if (cos_theta > 1.0) cos_theta = 1.0;
            value_type theta = Math::acos (cos_theta);

            if (theta) {
              Point<value_type> curv = next_dir - cos_theta * dir; curv.normalise();
              value_type R = S.step_size / theta;
              next_pos = pos + R * (Math::sin (theta) * dir + (value_type(1.0)-cos_theta) * curv);
              value_type val = FOD (next_pos, next_dir);
              if (isnan (val) || val < S.threshold) return (NAN);

              for (size_t i = S.num_samples; i > 0; --i) {
                value_type a = (theta * i) / S.num_samples, cos_a = Math::cos (a), sin_a = sin (a);
                Point<value_type> x = pos + R * (sin_a * dir + (value_type(1.0) - cos_a) * curv);
                Point<value_type> t = cos_a * dir + sin_a * curv;
                value_type amp = FOD (x, t);
                if (isnan (val) || amp < S.threshold) return (NAN);
                val *= amp;
              }
              return (val);
            }
            else { // straight on:
              next_pos = pos + S.step_size * dir;
              value_type val = FOD (next_pos, dir);
              if (isnan (val) || val < S.threshold) return (NAN);

              for (size_t i = S.num_samples; i > 0; --i) {
                value_type f = (S.step_size * i) / S.num_samples;
                Point<value_type> x = pos + f * dir;
                value_type amp = FOD (x, dir);
                if (isnan (val) || amp < S.threshold) return (NAN);
                val *= amp;
              }
              return (val);
            }
          }

          Point<value_type> rand_dir (const Point<value_type>& d) { return (random_direction (d, S.max_angle, S.sin_max_angle)); }
      };

    }
  }
}

#endif

