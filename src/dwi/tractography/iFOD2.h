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
#include "dwi/tractography/calibrator.h"


namespace MR {
  namespace DWI {
    namespace Tractography {

      class iFOD2 : public MethodBase {
        public:


          class Shared : public SharedBase {
            public:
              Shared (const std::string& source_name, DWI::Tractography::Properties& property_set) :
                SharedBase (source_name, property_set),
                lmax (Math::SH::LforN (source_buffer.dim(3))), 
                num_samples (4),
                max_trials (MAX_TRIALS),
                mean_samples (0.0),
                mean_num_truncations (0.0),
                max_max_truncation (0.0),
                num_proc (0) {

                  if (rk4)
                    throw Exception ("4th-order Runge-Kutta integration not valid for iFOD2 algorithm");

                  set_step_size (0.5);
                  sin_max_angle = Math::sin (max_angle);
                  INFO ("minimum radius of curvature = " + str(step_size / max_angle) + " mm");

                  properties["method"] = "iFOD2";
                  properties.set (lmax, "lmax");
                  properties.set (num_samples, "samples_per_step");
                  properties.set (max_trials, "max_trials");
                  fod_power = 1.0/num_samples;
                  properties.set (fod_power, "fod_power");
                  bool precomputed = true;
                  properties.set (precomputed, "sh_precomputed");
                  if (precomputed) precomputer.init (lmax);

                  // num_samples is number of samples excluding first point:
                  --num_samples;
                }

              ~Shared ()
              {
                INFO ("mean number of samples per step = " + str (mean_samples/double(num_proc))); 
                INFO ("mean number of rejection sampling truncations per step = " + str (mean_num_truncations/double(num_proc))); 
                INFO ("maximum truncation error = " + str (max_max_truncation)); 
              }

              void update_stats (double mean_samples_per_run, double num_truncations, double max_truncation) const 
              {
                mean_samples += mean_samples_per_run;
                mean_num_truncations += num_truncations;
                if (max_truncation > max_max_truncation) 
                  max_max_truncation = max_truncation;
                ++num_proc;
              }

              size_t lmax, num_samples, max_trials;
              value_type sin_max_angle, fod_power;
              Math::SH::PrecomputedAL<value_type> precomputer;

            private:
              mutable double mean_samples, mean_num_truncations, max_max_truncation;
              mutable int num_proc;
          };









          iFOD2 (const Shared& shared) :
            MethodBase (shared), 
            S (shared), 
            source (S.source_voxel),
            mean_sample_num (0), 
            num_sample_runs (0),
            num_truncations (0),
            max_truncation (0.0) {
              calibrate (*this);
            } 



          ~iFOD2 () 
          { 
            S.update_stats (calibrate_list.size() + value_type(mean_sample_num)/value_type(num_sample_runs), 
                value_type(num_truncations)/value_type(num_sample_runs),
                max_truncation);
          }




          bool init () 
          { 
            if (!get_data (source)) 
              return false;

            if (!S.init_dir) {
              for (size_t n = 0; n < S.max_trials; n++) {
                dir.set (rng.normal(), rng.normal(), rng.normal());
                dir.normalise();
                half_log_prob0 = FOD (dir);
                if (finite (half_log_prob0)) 
                  if (half_log_prob0 > S.init_threshold) 
                    goto end_init;
              }   
            }   
            else {
              dir = S.init_dir;
              half_log_prob0 = FOD (dir);
              if (finite (half_log_prob0)) 
                if (half_log_prob0 > S.init_threshold) 
                  goto end_init;
            }   

            return false;

end_init:
            half_log_prob0_seed = half_log_prob0 = 0.5 * Math::log (half_log_prob0);
            return true;
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
              return false;

            max_val *= calibrate_ratio;

            num_sample_runs++;

            for (size_t n = 0; n < S.max_trials; n++) {
              value_type val = rand_path_prob (next_pos, next_dir);

              if (val > max_val) {
                DEBUG ("max_val exceeded!!! (val = " + str(val) + ", max_val = " + str (max_val) + ")");
                  ++num_truncations;
                  if (val/max_val > max_truncation)
                    max_truncation = val/max_val;
              }

              if (rng.uniform() < val/max_val) {
                dir = next_dir;
                pos = next_pos;
                mean_sample_num += n;
                half_log_prob0 = last_half_log_probN;
                return true;
              }
            }

            return false;
          }


          void reverse_track()
          {
            half_log_prob0 = half_log_prob0_seed;
          }


          private:
          const Shared& S;
          Interpolator<SourceBufferType::voxel_type>::type source;
          value_type calibrate_ratio, half_log_prob0, last_half_log_probN, half_log_prob0_seed;
          size_t mean_sample_num, num_sample_runs, num_truncations;
          value_type max_truncation;
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
            if (!get_data (source, position)) 
              return NAN;
            return FOD (direction);
          }

          value_type rand_path_prob (Point<value_type>& next_pos, Point<value_type>& next_dir) 
          {
            Point<value_type> positions [S.num_samples], tangents [S.num_samples];
            get_path (positions, tangents, rand_dir (dir));

            value_type prob = path_prob (positions, tangents);

            next_pos = positions[S.num_samples-1];
            next_dir = tangents[S.num_samples-1];

            return prob;
          }


          value_type path_prob (Point<value_type>* positions, Point<value_type>* tangents)
          {
            value_type log_prob = half_log_prob0;
            for (size_t i = 0; i < S.num_samples; ++i) {
              value_type fod_amp = FOD (positions[i], tangents[i]);
              if (isnan (fod_amp) || fod_amp < S.threshold) 
                return NAN;
              fod_amp = Math::log (fod_amp);
              if (i < S.num_samples-1) log_prob += fod_amp;
              else {
                last_half_log_probN = 0.5*fod_amp;
                log_prob += last_half_log_probN;
              }
            }

            return Math::exp (S.fod_power * log_prob);
          }





          void get_path (Point<value_type>* positions, Point<value_type>* tangents, const Point<value_type>& end_dir) const
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
              for (size_t i = 0; i < S.num_samples; ++i) {
                value_type f = (i+1) * (S.step_size / S.num_samples);
                *positions++ = pos + f * dir;
                *tangents++ = dir;
              }
            }
          }

          Point<value_type> rand_dir (const Point<value_type>& d) { return (random_direction (d, S.max_angle, S.sin_max_angle)); }






          class Calibrate
          {
            public:
              Calibrate (iFOD2& method) : 
                P (method),
                fod (&P.values[0], P.source.dim(3))
              {
                Math::SH::delta (fod, Point<value_type> (0.0, 0.0, 1.0), P.S.lmax);
                init_log_prob = 0.5 * Math::log (Math::SH::value (P.values, Point<value_type> (0.0, 0.0, 1.0), P.S.lmax));
              }

              value_type operator() (value_type el) 
              {
                Point<value_type> positions [P.S.num_samples], tangents [P.S.num_samples];
                P.get_path (positions, tangents, Point<value_type> (Math::sin (el), 0.0, Math::cos(el)));
                
                value_type log_prob = init_log_prob;
                for (size_t i = 0; i < P.S.num_samples; ++i) {
                  value_type prob = Math::SH::value (P.values, tangents[i], P.S.lmax);
                  if (prob <= 0.0)
                    return 0.0;
                  prob = Math::log (prob);
                  if (i < P.S.num_samples-1) log_prob += prob;
                  else log_prob += 0.5*prob;
                }

                return Math::exp (P.S.fod_power * log_prob);
              }

            private:
              iFOD2& P;
              Math::Vector<value_type> fod;
              value_type init_log_prob;
          };

          friend void calibrate<iFOD2> (iFOD2& method);

      };

    }
  }
}

#endif

