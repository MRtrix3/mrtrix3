/*
 * Copyright (c) 2008-2018 the MRtrix3 contributors.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, you can obtain one at http://mozilla.org/MPL/2.0/
 *
 * MRtrix3 is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * For more details, see http://www.mrtrix.org/
 */


#ifndef __dwi_tractography_algorithms_iFOD2_h__
#define __dwi_tractography_algorithms_iFOD2_h__

#include <algorithm>

#include "types.h"
#include "math/SH.h"
#include "dwi/tractography/properties.h"
#include "dwi/tractography/tracking/method.h"
#include "dwi/tractography/tracking/shared.h"
#include "dwi/tractography/tracking/tractography.h"
#include "dwi/tractography/tracking/types.h"
#include "dwi/tractography/algorithms/calibrator.h"


#define TCKGEN_DEFAULT_IFOD2_NSAMPLES 4



namespace MR
{
  namespace DWI
  {
    namespace Tractography
    {
      namespace Algorithms
      {

        extern const App::OptionGroup iFOD2Option;
        void load_iFOD2_options (Tractography::Properties&);

        using namespace MR::DWI::Tractography::Tracking;

        class iFOD2 : public MethodBase { MEMALIGN(iFOD2)
          public:

            class Shared : public SharedBase { MEMALIGN(Shared)
              public:
                Shared (const std::string& diff_path, DWI::Tractography::Properties& property_set) :
                  SharedBase (diff_path, property_set),
                  lmax (Math::SH::LforN (source.size(3))),
                  num_samples (TCKGEN_DEFAULT_IFOD2_NSAMPLES),
                  max_trials (TCKGEN_DEFAULT_MAX_TRIALS_PER_STEP),
                  sin_max_angle (std::sin (max_angle)),
                  mean_samples (0.0),
                  mean_truncations (0.0),
                  max_max_truncation (0.0),
                  num_proc (0)
              {
                try {
                  Math::SH::check (source);
                } catch (Exception& e) {
                  e.display();
                  throw Exception ("Algorithm iFOD2 expects as input a spherical harmonic (SH) image");
                }

                if (rk4)
                  throw Exception ("4th-order Runge-Kutta integration not valid for iFOD2 algorithm");

                set_step_size (0.5);
                INFO ("minimum radius of curvature = " + str(step_size / (max_angle / Math::pi_2)) + " mm");

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
                const float min_dist = to<float> (properties["min_dist"]);
                min_num_points = std::max (2, Math::round<int> (min_dist/internal_step_size()) + 1);
                const float max_dist = to<float> (properties["max_dist"]);
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

                float internal_step_size() const override { return step_size / float(num_samples); }

                size_t lmax, num_samples, max_trials;
                float sin_max_angle, fod_power;
                Math::SH::PrecomputedAL<float> precomputer;

              private:
                mutable double mean_samples, mean_truncations, max_max_truncation;
                mutable int num_proc;
            };









            iFOD2 (const Shared& shared) :
              MethodBase (shared),
              S (shared),
              source (S.source),
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
              source (S.source),
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
              S.update_stats (calibrate_list.size() + float(mean_sample_num)/float(num_sample_runs),
                  float(num_truncations) / float(num_sample_runs),
                  max_truncation);
            }




            bool init()
            {
              if (!get_data (source))
                return false;

              if (!S.init_dir.allFinite()) {

                const Eigen::Vector3f init_dir (dir);

                for (size_t n = 0; n < S.max_seed_attempts; n++) {
                  dir = init_dir.allFinite() ? rand_dir (init_dir) : random_direction();
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
              half_log_prob0_seed = half_log_prob0 = 0.5 * std::log (half_log_prob0);
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

              Eigen::Vector3f next_pos, next_dir;

              float max_val = 0.0;
              for (size_t i = 0; i < calibrate_list.size(); ++i) {
                get_path (calib_positions, calib_tangents, rotate_direction (dir, calibrate_list[i]));
                float val = path_prob (calib_positions, calib_tangents);
                if (std::isnan (val))
                  return EXIT_IMAGE;
                else if (val > max_val)
                  max_val = val;
              }

              if (max_val <= 0.0)
                return CALIBRATOR;

              max_val *= calibrate_ratio;

              num_sample_runs++;

              for (size_t n = 0; n < S.max_trials; n++) {
                float val = rand_path_prob ();

                if (val > max_val) {
                  DEBUG ("max_val exceeded!!! (val = " + str(val) + ", max_val = " + str (max_val) + ")");
                  ++num_truncations;
                  if (val/max_val > max_truncation)
                    max_truncation = val/max_val;
                }

                if (uniform(*rng) < val/max_val) {
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


            FORCE_INLINE float get_metric()
            {
              return FOD (dir);
            }


            // Restore proper probability from the FOD at the track seed point
            void reverse_track() override
            {
              half_log_prob0 = half_log_prob0_seed;
              sample_idx = S.num_samples;
              MethodBase::reverse_track();
            }


            void truncate_track (GeneratedTrack& tck, const size_t length_to_revert_from, const size_t revert_step)
            {
              // OK, if we know length_to_revert_from, we can reconstruct what sample_idx was at that point
              size_t sample_idx_at_full_length = (length_to_revert_from - tck.get_seed_index()) % S.num_samples;
              // Unfortunately can't distinguish between sample_idx = 0 and sample_idx = S.num_samples
              // However the former would result in zero truncation with revert_step = 1...
              if (!sample_idx_at_full_length)
                sample_idx_at_full_length = S.num_samples;
              const size_t points_to_remove = sample_idx_at_full_length + ((revert_step - 1) * S.num_samples);
              if (tck.get_seed_index() + points_to_remove >= tck.size()) {
                tck.clear();
                pos = { NaN, NaN, NaN };
                dir = { NaN, NaN, NaN };
                return;
              }
              const size_t new_size = length_to_revert_from - points_to_remove;
              if (tck.size() == 2 || new_size == 1)
                dir = (tck[1] - tck[0]).normalized();
              else if (new_size != tck.size())
                dir = (tck[new_size] - tck[new_size - 2]).normalized();
              tck.resize (new_size);

              // Need to get the path probability contribution from the FOD at this point
              pos = tck.back();
              get_data (source);
              half_log_prob0 = 0.5 * std::log (FOD (dir));

              // Make sure that arc is re-calculated when next() is called
              sample_idx = S.num_samples;

              // Need to update sgm_depth appropriately, remembering that it is tracked by exec
              act().sgm_depth = (act().sgm_depth > points_to_remove) ? act().sgm_depth - points_to_remove : 0;
            }



          private:
            const Shared& S;
            Interpolator<Image<float>>::type source;
            float calibrate_ratio, half_log_prob0, last_half_log_probN, half_log_prob0_seed;
            size_t mean_sample_num, num_sample_runs, num_truncations;
            float max_truncation;
            vector<Eigen::Vector3f> calibrate_list;

            // Store list of points in the currently-calculated arc
            vector<Eigen::Vector3f> positions, calib_positions;
            vector<Eigen::Vector3f> tangents, calib_tangents;

            // Generate an arc only when required, and on the majority of next() calls, simply return the next point
            //   in the arc - more dense structural image sampling
            size_t sample_idx;



            FORCE_INLINE float FOD (const Eigen::Vector3f& direction) const
            {
              return (S.precomputer ?
                  S.precomputer.value (values, direction) :
                  Math::SH::value (values, direction, S.lmax)
                  );
            }

            FORCE_INLINE float FOD (const Eigen::Vector3f& position, const Eigen::Vector3f& direction)
            {
              if (!get_data (source, position))
                return NaN;
              return FOD (direction);
            }




            FORCE_INLINE float rand_path_prob ()
            {
              get_path (positions, tangents, rand_dir (dir));
              return path_prob (positions, tangents);
            }



            float path_prob (vector<Eigen::Vector3f>& positions, vector<Eigen::Vector3f>& tangents)
            {

              // Early exit for ACT when path is not sensible
              if (S.is_act()) {
                if (!act().fetch_tissue_data (positions[S.num_samples - 1]))
                  return (NaN);
                if (act().tissues().get_csf() >= 0.5)
                  return 0.0;
              }

              float log_prob = half_log_prob0;
              for (size_t i = 0; i < S.num_samples; ++i) {

                float fod_amp = FOD (positions[i], tangents[i]);
                if (std::isnan (fod_amp))
                  return NaN;
                if (fod_amp < S.threshold)
                  return 0.0;
                fod_amp = std::log (fod_amp);
                if (i < S.num_samples-1) {
                  log_prob += fod_amp;
                } else {
                  last_half_log_probN = 0.5*fod_amp;
                  log_prob += last_half_log_probN;
                }
              }

              return std::exp (S.fod_power * log_prob);
            }


          protected:
            void get_path (vector<Eigen::Vector3f>& positions, vector<Eigen::Vector3f>& tangents, const Eigen::Vector3f& end_dir) const
            {
              float cos_theta = end_dir.dot (dir);
              cos_theta = std::min (cos_theta, float(1.0));
              float theta = std::acos (cos_theta);

              if (theta) {

                Eigen::Vector3f curv = end_dir - cos_theta * dir;
                curv.normalize();
                float R = S.step_size / theta;

                for (size_t i = 0; i < S.num_samples-1; ++i) {
                  float a = (theta * (i+1)) / S.num_samples;
                  float cos_a = std::cos (a);
                  float sin_a = std::sin (a);
                  positions[i] = pos + R * (sin_a * dir + (float(1.0) - cos_a) * curv);
                  tangents[i] = cos_a * dir + sin_a * curv;
                }
                positions[S.num_samples-1] = pos + R * (std::sin (theta) * dir + (float(1.0)-cos_theta) * curv);
                tangents[S.num_samples-1]  = end_dir;

              } else { // straight on:

                for (size_t i = 0; i < S.num_samples; ++i) {
                  float f = (i+1) * (S.step_size / S.num_samples);
                  positions[i] = pos + f * dir;
                  tangents[i]  = dir;
                }

              }
            }



            FORCE_INLINE Eigen::Vector3f rand_dir (const Eigen::Vector3f& d) { return (random_direction (d, S.max_angle, S.sin_max_angle)); }



          private:
            class Calibrate
            { MEMALIGN(Calibrate)
              public:
                Calibrate (iFOD2& method) :
                  P (method),
                  fod (P.values),
                  vox (P.S.vox()),
                  positions (P.S.num_samples),
                  tangents (P.S.num_samples) {
                    Math::SH::delta (fod, Eigen::Vector3f (0.0, 0.0, 1.0), P.S.lmax);
                    init_log_prob = 0.5 * std::log (Math::SH::value (P.values, Eigen::Vector3f (0.0, 0.0, 1.0), P.S.lmax));
                  }

                float operator() (float el)
                {
                  P.pos = { 0.0f, 0.0f, 0.0f };
                  P.get_path (positions, tangents, Eigen::Vector3f (std::sin (el), 0.0, std::cos(el)));

                  float log_prob = init_log_prob;
                  for (size_t i = 0; i < P.S.num_samples; ++i) {
                    float prob = Math::SH::value (P.values, tangents[i], P.S.lmax) * (1.0 - (positions[i][0] / vox));
                    if (prob <= 0.0)
                      return 0.0;
                    prob = std::log (prob);
                    if (i < P.S.num_samples-1)
                      log_prob += prob;
                    else
                      log_prob += 0.5*prob;
                  }

                  return std::exp (P.S.fod_power * log_prob);
                }

              private:
                iFOD2& P;
                Eigen::VectorXf& fod;
                const float vox;
                float init_log_prob;
                vector<Eigen::Vector3f> positions, tangents;
            };

            friend void calibrate<iFOD2> (iFOD2& method);

        };



      }
    }
  }
}

#endif

