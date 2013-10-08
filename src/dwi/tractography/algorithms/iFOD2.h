/*
    Copyright 2011 Brain Research Institute, Melbourne, Australia

    Written by J-Donald Tournier and Robert E. Smith, 2011.

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
#include "dwi/tractography/algorithms/calibrator.h"




namespace MR
{
  namespace DWI
  {
    namespace Tractography
    {

    class iFOD2 : public MethodBase {
      public:

      class Shared : public SharedBase {
        public:
        Shared (const std::string& diff_path, DWI::Tractography::Properties& property_set) :
          SharedBase (diff_path, property_set),
          lmax (Math::SH::LforN (source_buffer.dim(3))),
          num_samples (4),
          max_trials (MAX_TRIALS),
          sin_max_angle (Math::sin (max_angle)),
          mean_samples (0.0),
          mean_truncations (0.0),
          max_max_truncation (0.0),
          num_proc (0)
        {

          if (rk4)
            throw Exception ("4th-order Runge-Kutta integration not valid for iFOD2 algorithm");

          set_step_size (0.5);
          INFO ("minimum radius of curvature = " + str(step_size / (max_angle / M_PI_2)) + " mm");

          properties["method"] = "iFOD2";
          properties.set (lmax, "lmax");
          properties.set (num_samples, "samples_per_step");
          properties.set (max_trials, "max_trials");
          fod_power = 1.0/num_samples;
          properties.set (fod_power, "fod_power");
          bool precomputed = true;
          properties.set (precomputed, "sh_precomputed");
          if (precomputed)
            precomputer.init (lmax);

          // num_samples is number of samples excluding first point
          --num_samples;

          INFO ("iFOD2 internal step size = " + str (internal_step_size()) + " mm");

          // Have to modify length criteria, as they are enforced in points, not mm
          const value_type min_dist = to<value_type> (properties["min_dist"]);
          min_num_points = std::max (2, round (min_dist/internal_step_size()) + 1);
          const value_type max_dist = to<value_type> (properties["max_dist"]);
          max_num_points = round (max_dist/internal_step_size()) + 1;

          // iFOD2 by default downsamples after track propagation back to the desired 'step size'
          //   i.e. the sub-step detail is removed from the output
          downsample = num_samples;
          properties.set (downsample, "downsample_factor");

          properties["output_step_size"] = str (step_size * downsample / float(num_samples));

        }

        ~Shared ()
        {
          mean_samples /= double(num_proc);
          mean_truncations /= double(num_proc);
          INFO ("mean number of samples per step = " + str (mean_samples));
          if (mean_truncations) {
            INFO ("mean number of steps between rejection sampling truncations = " + str (1.0/mean_truncations));
            INFO ("maximum truncation error = " + str (max_max_truncation));
          } else {
            INFO ("no rejection sampling truncations occurred");
          }
        }

        void update_stats (double mean_samples_per_run, double mean_truncations_per_run, double max_truncation) const
        {
          mean_samples += mean_samples_per_run;
          mean_truncations += mean_truncations_per_run;
          if (max_truncation > max_max_truncation)
            max_max_truncation = max_truncation;
          ++num_proc;
        }

        float internal_step_size() const { return step_size / float(num_samples); }

        size_t lmax, num_samples, max_trials;
        value_type sin_max_angle, fod_power;
        Math::SH::PrecomputedAL<value_type> precomputer;

        private:
        mutable double mean_samples, mean_truncations, max_max_truncation;
        mutable int num_proc;
      };









      iFOD2 (const Shared& shared) :
        MethodBase (shared),
        S (shared),
        source (S.source_voxel),
        mean_sample_num (0),
        num_sample_runs (0),
        num_truncations (0),
        max_truncation (0.0),
        positions (new Point<value_type>[S.num_samples]),
        tangents  (new Point<value_type>[S.num_samples]),
        sample_idx (S.num_samples)
      {
        calibrate (*this);
        for (size_t i = 0; i != S.num_samples; ++i)
          positions[i] = tangents[i] = Point<float>();
      }

      iFOD2 (const iFOD2& that) :
        MethodBase (that.S),
        S (that.S),
        source (S.source_voxel),
        calibrate_ratio (that.calibrate_ratio),
        mean_sample_num (0),
        num_sample_runs (0),
        num_truncations (0),
        max_truncation (0.0),
        calibrate_list (that.calibrate_list),
        positions (new Point<value_type>[S.num_samples]),
        tangents  (new Point<value_type>[S.num_samples]),
        sample_idx (S.num_samples)
      {
        for (size_t i = 0; i != S.num_samples; ++i)
          positions[i] = tangents[i] = Point<float>();
      }



      ~iFOD2 ()
      {
        S.update_stats (calibrate_list.size() + value_type(mean_sample_num)/value_type(num_sample_runs),
                        value_type(num_truncations) / value_type(num_sample_runs),
                        max_truncation);
        delete[] positions;
        delete[] tangents;
      }




      bool init ()
      {
        if (!get_data (source))
          return false;

        if (!S.init_dir) {

          const Point<float> init_dir (dir);

          for (size_t n = 0; n < S.max_trials; n++) {
            if (init_dir.valid()) {
              dir = rand_dir (init_dir);
            } else {
              dir.set (rng.normal(), rng.normal(), rng.normal());
              dir.normalise();
            }
            half_log_prob0 = FOD (dir);
            if (finite (half_log_prob0) && (half_log_prob0 > S.init_threshold))
              goto end_init;
          }

        } else {

          dir = S.init_dir;
          half_log_prob0 = FOD (dir);
          if (finite (half_log_prob0) && (half_log_prob0 > S.init_threshold))
            goto end_init;

        }

        return false;

        end_init:
        half_log_prob0_seed = half_log_prob0 = 0.5 * Math::log (half_log_prob0);
        sample_idx = S.num_samples; // Force arc calculation on first iteration
        return true;
      }



      term_t next ()
      {

        if (++sample_idx < S.num_samples) {
          pos = positions[sample_idx];
          dir = tangents [sample_idx];
          return CONTINUE;
        }

        Point<value_type> next_pos, next_dir;
        Point<value_type> calib_positions [S.num_samples], calib_tangents [S.num_samples];

        value_type max_val = 0.0;
        size_t nan_count = 0;
        for (size_t i = 0; i < calibrate_list.size(); ++i) {
          get_path (calib_positions, calib_tangents, rotate_direction (dir, calibrate_list[i]));
          value_type val = path_prob (calib_positions, calib_tangents);
          if (isnan (val))
            ++nan_count;
          else if (val > max_val)
            max_val = val;
        }

        if (nan_count == calibrate_list.size())
          return EXIT_IMAGE;

        if (max_val <= 0.0)
          return CALIBRATE_FAIL;

        max_val *= calibrate_ratio;

        num_sample_runs++;

        for (size_t n = 0; n < S.max_trials; n++) {
          value_type val = rand_path_prob ();

          if (val > max_val) {
            DEBUG ("max_val exceeded!!! (val = " + str(val) + ", max_val = " + str (max_val) + ")");
            ++num_truncations;
            if (val/max_val > max_truncation)
              max_truncation = val/max_val;
          }

          if (rng.uniform() < val/max_val) {
            mean_sample_num += n;
            half_log_prob0 = last_half_log_probN;
            pos = positions[0];
            dir = tangents [0];
            sample_idx = 0;
            return CONTINUE;
          }
        }

        return BAD_SIGNAL;
      }


      float get_metric()
      {
        return FOD (dir);
      }


      // Restore proper probability from the FOD at the track seed point
      void reverse_track()
      {
        half_log_prob0 = half_log_prob0_seed;
        sample_idx = S.num_samples;
      }


      void truncate_track (Track& tck, const int revert_step)
      {
        // Need to be able to get an estimate of the tangent at the new endpoint
        // Removing start of current arc (counts as 1 if it exists) plus 1 arclength for each remaining revert_step
        const int points_to_remove = (sample_idx ? sample_idx : S.num_samples) + ((revert_step - 1) * S.num_samples);
        const int new_end_idx = (int)tck.size() - 1 - points_to_remove;
        if (new_end_idx <= 1) {
          tck.clear();
          pos.invalidate();
          dir.invalidate();
          return;
        }
        dir = (tck[new_end_idx + 1] - tck[new_end_idx - 1]).normalise();

        // Erase the track up to the correct point
        tck.erase (tck.begin() + new_end_idx + 1, tck.end());

        // Need to get the path probability contribution from the FOD at this point
        pos = tck.back();
        get_data (source);
        half_log_prob0 = 0.5 * Math::log (FOD (dir));

        // Make sure that arc is re-calculated when next() is called
        sample_idx = S.num_samples;

        // Need to update sgm_depth appropriately, remembering that it is tracked by exec
        act().sgm_depth = MAX (0, act().sgm_depth - points_to_remove);
      }



      private:
      const Shared& S;
      Interpolator<SourceBufferType::voxel_type>::type source;
      value_type calibrate_ratio, half_log_prob0, last_half_log_probN, half_log_prob0_seed;
      size_t mean_sample_num, num_sample_runs, num_truncations;
      value_type max_truncation;
      std::vector< Point<value_type> > calibrate_list;

      // Store list of points in the currently-calculated arc
      Point<value_type>* positions;
      Point<value_type>* tangents;

      // Generate an arc only when required, and on the majority of next() calls, simply return the next point
      //   in the arc - more dense structural image sampling
      size_t sample_idx;



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




      value_type rand_path_prob ()
      {
        get_path (positions, tangents, rand_dir (dir));
        return path_prob (positions, tangents);
      }



      value_type path_prob (Point<value_type>* positions, Point<value_type>* tangents)
      {

        // Early exit for ACT when path is not sensible
        if (S.is_act()) {
          if (!act().fetch_tissue_data (positions[S.num_samples - 1]))
            return (NAN);
          if (act().tissues().get_csf() >= 0.5)
            return 0.0;
        }

        value_type log_prob = half_log_prob0;
        for (size_t i = 0; i < S.num_samples; ++i) {

          value_type fod_amp = FOD (positions[i], tangents[i]);
          if (isnan (fod_amp))
            return (NAN);
          if (fod_amp < S.threshold)
            return 0.0;
          fod_amp = Math::log (fod_amp);
          if (i < S.num_samples-1) {
            log_prob += fod_amp;
          } else {
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
            *tangents++  = cos_a * dir + sin_a * curv;
          }
          *positions = pos + R * (Math::sin (theta) * dir + (value_type(1.0)-cos_theta) * curv);
          *tangents  = end_dir;

        } else { // straight on:

          for (size_t i = 0; i < S.num_samples; ++i) {
            value_type f = (i+1) * (S.step_size / S.num_samples);
            *positions++ = pos + f * dir;
            *tangents++  = dir;
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
              if (i < P.S.num_samples-1)
                log_prob += prob;
              else
                log_prob += 0.5*prob;
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

