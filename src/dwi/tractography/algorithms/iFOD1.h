/* Copyright (c) 2008-2017 the MRtrix3 contributors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, you can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * MRtrix is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * For more details, see http://www.mrtrix.org/.
 */


#ifndef __dwi_tractography_algorithms_iFOD1_h__
#define __dwi_tractography_algorithms_iFOD1_h__

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

    class iFOD1 : public MethodBase { MEMALIGN(iFOD1)
      public:
      class Shared : public SharedBase { MEMALIGN(Shared)
        public:
        Shared (const std::string& diff_path, DWI::Tractography::Properties& property_set) :
          SharedBase (diff_path, property_set),
          lmax (Math::SH::LforN (source.size(3))),
          max_trials (MAX_TRIALS),
          sin_max_angle (std::sin (max_angle)),
          mean_samples (0.0),
          mean_truncations (0.0),
          max_max_truncation (0.0),
          num_proc (0) {

          try {
            Math::SH::check (source);
          } catch (Exception& e) {
            e.display();
            throw Exception ("Algorithm iFOD1 expects as input a spherical harmonic (SH) image");
          }

          set_step_size (0.1);
          if (rk4) {
            max_angle = 0.5 * max_angle_rk4;
            INFO ("minimum radius of curvature = " + str(step_size / (max_angle_rk4 / (0.5 * Math::pi))) + " mm");
          } else {
            INFO ("minimum radius of curvature = " + str(step_size / ( 2.0 * sin (max_angle / 2.0))) + " mm");
          }
          sin_max_angle = std::sin (max_angle);

          properties["method"] = "iFOD1";
          properties.set (lmax, "lmax");
          properties.set (max_trials, "max_trials");
          bool precomputed = true;
          properties.set (precomputed, "sh_precomputed");
          if (precomputed)
            precomputer.init (lmax);

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

        size_t lmax, max_trials;
        float sin_max_angle;
        Math::SH::PrecomputedAL<float> precomputer;

        private:
        mutable double mean_samples, mean_truncations, max_max_truncation;
        mutable int num_proc;
      };






      iFOD1 (const Shared& shared) :
        MethodBase (shared),
        S (shared),
        source (S.source),
        mean_sample_num (0),
        num_sample_runs (0),
        num_truncations (0),
        max_truncation (0.0) {
        calibrate (*this);
      }


      ~iFOD1 ()
      {
        S.update_stats (calibrate_list.size() + float(mean_sample_num)/float(num_sample_runs),
                        float(num_truncations) / float(num_sample_runs),
                        max_truncation);
      }



      bool init()
      {
        if (!get_data (source))
          return (false);

        if (!S.init_dir.allFinite()) {

          const Eigen::Vector3f init_dir (dir);

          for (size_t n = 0; n < S.max_seed_attempts; n++) {
            dir = init_dir.allFinite() ? rand_dir (init_dir) : random_direction();
            float val = FOD (dir);
            if (std::isfinite (val))
              if (val > S.init_threshold)
                return true;
          }

        } 
        else {
          dir = S.init_dir;
          float val = FOD (dir);
          if (std::isfinite (val))
            if (val > S.init_threshold)
              return true;

        }

        return false;
      }



      term_t next ()
      {
        if (!get_data (source))
          return EXIT_IMAGE;

        float max_val = 0.0;
        for (size_t i = 0; i < calibrate_list.size(); ++i) {
          float val = FOD (rotate_direction (dir, calibrate_list[i]));
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
          Eigen::Vector3f new_dir = rand_dir (dir);
          float val = FOD (new_dir);

          if (val > S.threshold) {

            if (val > max_val) {
              DEBUG ("max_val exceeded!!! (val = " + str(val) + ", max_val = " + str (max_val) + ")");
              ++num_truncations;
              if (val/max_val > max_truncation)
                max_truncation = val/max_val;
            }

            if (uniform(*rng) < val/max_val) {
              dir = new_dir;
              dir.normalize();
              pos += S.step_size * dir;
              mean_sample_num += n;
              return CONTINUE;
            }

          }
        }

        return BAD_SIGNAL;
      }


      float get_metric()
      {
        return FOD (dir);
      }


      protected:
      const Shared& S;
      Interpolator<Image<float>>::type source;
      float calibrate_ratio;
      size_t mean_sample_num, num_sample_runs, num_truncations;
      float max_truncation;
      vector< Eigen::Vector3f > calibrate_list;

      float FOD (const Eigen::Vector3f& d) const
      {
        return (S.precomputer ?
            S.precomputer.value (values, d) :
            Math::SH::value (values, d, S.lmax)
        );
      }

      Eigen::Vector3f rand_dir (const Eigen::Vector3f& d) { return (random_direction (d, S.max_angle, S.sin_max_angle)); }





      class Calibrate
      { MEMALIGN (Calibrate)
        public:
          Calibrate (iFOD1& method) :
            P (method),
            fod (P.values)
          {
            Math::SH::delta (fod, Eigen::Vector3f (0.0, 0.0, 1.0), P.S.lmax);
          }

          float operator() (float el)
          {
            return Math::SH::value (P.values, Eigen::Vector3f (std::sin (el), 0.0, std::cos(el)), P.S.lmax);
          }

        private:
          iFOD1& P;
          Eigen::VectorXf& fod;
      };

      friend void calibrate<iFOD1> (iFOD1& method);

    };

      }
    }
  }
}

#endif

