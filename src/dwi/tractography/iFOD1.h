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

#ifndef __dwi_tractography_iFOD1_h__
#define __dwi_tractography_iFOD1_h__

#include "point.h"
#include "math/SH.h"
#include "dwi/tractography/method.h"
#include "dwi/tractography/shared.h"
#include "dwi/tractography/calibrator.h"


namespace MR {
  namespace DWI {
    namespace Tractography {

      class iFOD1 : public MethodBase {
        public:
          class Shared : public SharedBase {
            public:
              Shared (const std::string& source_name, DWI::Tractography::Properties& property_set) :
                SharedBase (source_name, property_set),
                lmax (Math::SH::LforN (source.dim(3))), 
                max_trials (MAX_TRIALS),
                sin_max_angle (Math::sin (max_angle)),
                mean_samples (0.0),
                mean_num_truncations (0.0),
                max_max_truncation (0.0),
                num_proc (0) {

                  set_step_size (0.1);
                  if (rk4) {
                    max_angle = 0.5 * max_angle_rk4;
                    inform ("minimum radius of curvature = " + str(step_size / (max_angle_rk4 / (0.5 * M_PI))) + " mm");
                  } else {
                    inform ("minimum radius of curvature = " + str(step_size / ( 2.0 * sin (max_angle / 2.0))) + " mm");
                  }
                  sin_max_angle = Math::sin (max_angle);

                  properties["method"] = "iFOD1";
                  properties.set (lmax, "lmax");
                  properties.set (max_trials, "max_trials");
                  bool precomputed = true;
                  properties.set (precomputed, "sh_precomputed");
                  if (precomputed) precomputer.init (lmax);
                }

              ~Shared ()
              {
                inform ("mean number of samples per step = " + str (mean_samples/double(num_proc))); 
                inform ("mean number of rejection sampling truncations per step = " + str (mean_num_truncations/double(num_proc))); 
                inform ("maximum truncation error = " + str (max_max_truncation)); 
              }

              void update_stats (double mean_samples_per_run, double num_truncations, double max_truncation) const 
              {
                mean_samples += mean_samples_per_run;
                mean_num_truncations += num_truncations;
                if (max_truncation > max_max_truncation) 
                  max_max_truncation = max_truncation;
                ++num_proc;
              }

              size_t lmax, max_trials;
              value_type sin_max_angle;
              Math::SH::PrecomputedAL<value_type> precomputer;

            private:
              mutable double mean_samples, mean_num_truncations, max_max_truncation;
              mutable int num_proc;
          };






          iFOD1 (const Shared& shared) : 
            MethodBase (shared), 
            S (shared),
            mean_sample_num (0), 
            num_sample_runs (0),
            num_truncations (0),
            max_truncation (0.0) {
              calibrate (*this);
            } 


          ~iFOD1 () 
          { 
            S.update_stats (calibrate_list.size() + value_type(mean_sample_num)/value_type(num_sample_runs), 
                value_type(num_truncations)/value_type(num_sample_runs),
                max_truncation);
          }



          bool init () 
          { 
            if (!get_data ()) return (false);

            if (!S.init_dir) {
              for (size_t n = 0; n < S.max_trials; n++) {
                dir.set (rng.normal(), rng.normal(), rng.normal());
                dir.normalise();
                value_type val = FOD (dir);
                if (finite (val)) 
                  if (val > S.init_threshold) 
                    return (true);
              }   
            }   
            else {
              dir = S.init_dir;
              value_type val = FOD (dir);
              if (finite (val)) 
                if (val > S.init_threshold) 
                  return (true);
            }   

            return (false);
          }   



          bool next () 
          {
            if (!get_data ())
              return (false);

            value_type max_val = 0.0;
            for (size_t i = 0; i < calibrate_list.size(); ++i) {
              value_type val = FOD (rotate_direction (dir, calibrate_list[i]));
              if (val > max_val) 
                max_val = val;
            }

            if (max_val <= 0.0 || !finite (max_val))
              return (false);

            max_val *= calibrate_ratio;

            num_sample_runs++;

            for (size_t n = 0; n < S.max_trials; n++) {
              Point<value_type> new_dir = rand_dir (dir);
              value_type val = FOD (new_dir);

              if (val > S.threshold) {

                if (val > max_val) {
                  debug ("max_val exceeded!!! (val = " + str(val) + ", max_val = " + str (max_val) + ")");
                  ++num_truncations;
                  if (val/max_val > max_truncation)
                    max_truncation = val/max_val;
                }

                if (rng.uniform() < val/max_val) {
                  dir = new_dir;
                  dir.normalise();
                  pos += S.step_size * dir;
                  mean_sample_num += n;
                  return (true);
                }

              }
            }

            return (false);
          }

        protected:
          const Shared& S;
          value_type calibrate_ratio;
          size_t mean_sample_num, num_sample_runs, num_truncations;
          float max_truncation;
          std::vector<Point<value_type> > calibrate_list;

          value_type FOD (const Point<value_type>& d) const 
          {
            return (S.precomputer ?
                S.precomputer.value (values, d) :
                Math::SH::value (values, d, S.lmax)
                );
          }

          Point<value_type> rand_dir (const Point<value_type>& d) { return (random_direction (d, S.max_angle, S.sin_max_angle)); }





          class Calibrate
          {
            public:
              Calibrate (iFOD1& method) : 
                P (method),
                fod (P.values, P.source.dim(3))
              {
                Math::SH::delta (fod, Point<value_type> (0.0, 0.0, 1.0), P.S.lmax);
              }

              value_type operator() (value_type el) 
              {
                return Math::SH::value (P.values, Point<value_type> (Math::sin (el), 0.0, Math::cos(el)), P.S.lmax);
              }

            private:
              const iFOD1& P;
              Math::Vector<value_type> fod;
          };

          friend void calibrate<iFOD1> (iFOD1& method);

      };

    }
  }
}

#endif

