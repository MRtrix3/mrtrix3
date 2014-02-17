/*******************************************************************************
    Copyright (C) 2014 Brain Research Institute, Melbourne, Australia
    
    Permission is hereby granted under the Patent Licence Agreement between
    the BRI and Siemens AG from July 3rd, 2012, to Siemens AG obtaining a
    copy of this software and associated documentation files (the
    "Software"), to deal in the Software without restriction, including
    without limitation the rights to possess, use, develop, manufacture,
    import, offer for sale, market, sell, lease or otherwise distribute
    Products, and to permit persons to whom the Software is furnished to do
    so, subject to the following conditions:
    
    The above copyright notice and this permission notice shall be included
    in all copies or substantial portions of the Software.
    
    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
    OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
    MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
    IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
    CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
    TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
    SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

*******************************************************************************/


#ifndef __dwi_tractography_algorithms_iFOD2_h__
#define __dwi_tractography_algorithms_iFOD2_h__

#include <algorithm>

#include "point.h"
#include "math/SH.h"
#include "dwi/tractography/tracking/method.h"
#include "dwi/tractography/tracking/shared.h"
#include "dwi/tractography/tracking/types.h"
#include "dwi/tractography/algorithms/calibrator.h"




namespace MR
{
  namespace DWI
  {
    namespace Tractography
    {
      namespace Algorithms
      {

    using namespace MR::DWI::Tractography::Tracking;

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
          size_t downsample_ratio = num_samples;
          properties.set (downsample_ratio, "downsample_factor");
          downsampler.set_ratio (downsample_ratio);

          properties["output_step_size"] = str (step_size * downsample_ratio / float(num_samples));

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
        positions (S.num_samples),
        calib_positions (S.num_samples),
        tangents (S.num_samples),
        calib_tangents (S.num_samples),
        sample_idx (S.num_samples)
      {
        calibrate (*this);
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
        positions (S.num_samples),
        calib_positions (S.num_samples),
        tangents (S.num_samples),
        calib_tangents (S.num_samples),
        sample_idx (S.num_samples)
      {
      }



      ~iFOD2 ()
      {
        S.update_stats (calibrate_list.size() + value_type(mean_sample_num)/value_type(num_sample_runs),
                        value_type(num_truncations) / value_type(num_sample_runs),
                        max_truncation);
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
            if (std::isfinite (half_log_prob0) && (half_log_prob0 > S.init_threshold))
              goto end_init;
          }

        } else {

          dir = S.init_dir;
          half_log_prob0 = FOD (dir);
          if (std::isfinite (half_log_prob0) && (half_log_prob0 > S.init_threshold))
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


      // Restore proper probability from the FOD at the track seed point
      void reverse_track()
      {
        half_log_prob0 = half_log_prob0_seed;
        sample_idx = S.num_samples;
      }




      private:
      const Shared& S;
      Interpolator<SourceBufferType::voxel_type>::type source;
      value_type calibrate_ratio, half_log_prob0, last_half_log_probN, half_log_prob0_seed;
      size_t mean_sample_num, num_sample_runs, num_truncations;
      value_type max_truncation;
      std::vector< Point<value_type> > calibrate_list;

      // Store list of points in the currently-calculated arc
      std::vector< Point<value_type> > positions, calib_positions;
      std::vector< Point<value_type> > tangents, calib_tangents;

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



      value_type path_prob (std::vector< Point<value_type> >& positions, std::vector< Point<value_type> >& tangents)
      {

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



      void get_path (std::vector< Point<value_type> >& positions, std::vector< Point<value_type> >& tangents, const Point<value_type>& end_dir) const
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
            positions[i] = pos + R * (sin_a * dir + (value_type(1.0) - cos_a) * curv);
            tangents[i] = cos_a * dir + sin_a * curv;
          }
          positions[S.num_samples-1] = pos + R * (Math::sin (theta) * dir + (value_type(1.0)-cos_theta) * curv);
          tangents[S.num_samples-1]  = end_dir;

        } else { // straight on:

          for (size_t i = 0; i < S.num_samples; ++i) {
            value_type f = (i+1) * (S.step_size / S.num_samples);
            positions[i] = pos + f * dir;
            tangents[i]  = dir;
          }

        }
      }



      Point<value_type> rand_dir (const Point<value_type>& d) { return (random_direction (d, S.max_angle, S.sin_max_angle)); }




      class Calibrate
      {
        public:
          Calibrate (iFOD2& method) :
            P (method),
            fod (&P.values[0], P.source.dim(3)),
            positions (P.S.num_samples),
            tangents (P.S.num_samples)
          {
            Math::SH::delta (fod, Point<value_type> (0.0, 0.0, 1.0), P.S.lmax);
            init_log_prob = 0.5 * Math::log (Math::SH::value (P.values, Point<value_type> (0.0, 0.0, 1.0), P.S.lmax));
          }

          value_type operator() (value_type el)
          {
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
          std::vector< Point<value_type> > positions, tangents;
      };

      friend void calibrate<iFOD2> (iFOD2& method);

    };



      }
    }
  }
}

#endif

