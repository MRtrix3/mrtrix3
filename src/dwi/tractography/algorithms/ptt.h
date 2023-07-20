/* Copyright (c) 2008-2023 the MRtrix3 contributors.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Covered Software is provided under this License on an "as is"
 * basis, without warranty of any kind, either expressed, implied, or
 * statutory, including, without limitation, warranties that the
 * Covered Software is free of defects, merchantable, fit for a
 * particular purpose or non-infringing.
 * See the Mozilla Public License v. 2.0 for more details.
 *
 * For more details, see http://www.mrtrix.org/.
 */

#ifndef __dwi_tractography_algorithms_ptt_h__
#define __dwi_tractography_algorithms_ptt_h__

#include "math/SH.h"
#include "dwi/tractography/rng.h"
#include "dwi/tractography/tracking/method.h"
#include "dwi/tractography/tracking/shared.h"
#include "dwi/tractography/tracking/tractography.h"
#include "dwi/tractography/tracking/types.h"

namespace MR
{
  namespace DWI
  {
    namespace Tractography
    {
      namespace Algorithms
      {



        using namespace MR::DWI::Tractography::Tracking;
        extern const App::OptionGroup PTTOptions;
        void load_PTT_options (Tractography::Properties&);



        class PTT : public MethodBase
        {
        public:
          class Shared : public SharedBase
          {
          public:
            Shared (const std::string &diff_path, DWI::Tractography::Properties &property_set) :
                SharedBase (diff_path, property_set),
                lmax (Math::SH::LforN (source.size(3))),
                // TODO Provide command-line interfaces
                max_trials_calibration_tracking (20),
                max_trials_calibration_seeding (1000),
                max_trials_sampling (Defaults::max_trials_per_step),
                kmax (2.0f / vox()),
                nsamples (Defaults::secondorder_nsamples),
                probe_length (Defaults::probelength_voxels_ptt * vox())
            {
              try {
                Math::SH::check (source);
              } catch (Exception &e) {
                e.display();
                throw Exception ("Algorithm PTT expects as input a spherical harmonic (SH) image");
              }

              if (rk4)
                throw Exception ("4th-order Runge-Kutta integration not valid for PTT algorithm");

              properties["method"] = "PTT";

              // TODO Compute appropriate combination of angle and kmax
              // Don't think that angle should actually be that consequential,
              //   since it's ultimately kmax that controls maximum curvature of calculated trajectories;
              // however if the user manually specifies an angle, then technically kmax should be
              //   being computed from that
              // TODO Think that some of the calculations in set_step_and_angle() may be based on a
              //   fixed chord length rather than fixed arc length;
              //   this will be the case for set_num_points() as well
              // TODO In addition this needs to properly deal with potential differences between step size and probe length

              set_step_and_angle (Defaults::stepsize_voxels_ptt, Defaults::angle_ptt, true);
              properties.set (probe_length, "probe_length");
              set_num_points();

              set_cutoff (Defaults::cutoff_fod * (is_act() ? Defaults::cutoff_act_multiplier : 1.0));

              // Determine the downsampling factor required such that the exported step size is
              //   no larger than this number of voxels
              downsampler.set_ratio (std::floor (Defaults::downsample_stepsize_voxels_ptt / step_size));

              properties.set (max_trials_sampling, "max_trials");

              bool precomputed = true;
              properties.set (precomputed, "sh_precomputed");
              if (precomputed)
                precomputer = new Math::SH::PrecomputedAL<float>(lmax);

              properties.set (nsamples, "samples_per_step");
              probe_t.reserve (nsamples);
              for (size_t i = 0; i != nsamples; ++i)
                probe_t.push_back (probe_length * float(i) / float(nsamples-1));

            }

            ~Shared()
            {
              if (precomputer)
                delete precomputer;
            }

            size_t lmax;
            // TODO Homogenize support for trial count selection across algorithms
            // ("max_trials_sampling" now reads from same command-line option and default value as iFOD#;
            //   calibrators may change implementation in the future)
            size_t max_trials_calibration_tracking, max_trials_calibration_seeding, max_trials_sampling;
            float kmax;
            size_t nsamples;
            float probe_length;
            vector<float> probe_t;
            Math::SH::PrecomputedAL<float> *precomputer;
          };








          PTT (const Shared &shared) :
              MethodBase (shared),
              S (shared),
              source (S.source),
              probe (4) {}

          PTT (const PTT &that) :
              MethodBase (that.S),
              S (that.S),
              source (S.source),
              probe (4) {}

          ~PTT() {}



          // TODO Currently, the PTT algorithm as published generates an entire segment from the seed point
          //   in order to generate the PTF just at the seed point; but that segment is then discarded.
          //   This differs from iFOD2, which just looks for a supra-threshold FOD amplitude at the seed point and
          //   doesn't consider any higher-order / non-local effects.
          // Possible options:
          //   - Do similar for PTT, with random initialisation of K1 & K2.
          //   - Generate that initial segment, but additionally buffer it so that it is used to form the first
          //     segment upon the first call to next()
          //   - Ignore, and just accept that there's a single redundant segment calculated from the seed point
          bool init() override
          {
            F = F_seed = Eigen::Matrix<float, 4, 3>::Zero();
            for (size_t i = 0; i != S.nsamples; ++i)
              probe[i].setZero();

            if (!get_data(source))
              return (false);

            const f_and_kpair start = initialize (pos);
            dir = F.T();
            F_seed = F;
            return std::isfinite (start.f);
          }

          term_t next() override
          {
            if (!get_data (source))
              return EXIT_IMAGE;

            orthonormalize();

            const f_and_kpair sample = rejection_sample<false>();
            if (!std::isfinite (sample.f))
              return MODEL;

            const Eigen::Matrix<float, 4, 4> propagator = P (sample.k1, sample.k2, S.step_size);
            F = propagator * F;
            pos = F.x();
            dir = F.T();
            return CONTINUE;
          }

          void reverse_track() override
          {
            F.x (F_seed.x());
            F.T (-F_seed.T());
            F.K1 (-F_seed.K1());
            F.K2 (-F_seed.K2());
            F_seed.setZero();
          }

          float get_metric (const Eigen::Vector3f &position, const Eigen::Vector3f &direction) override
          {
            if (!get_data (source, position))
              return 0.0f;
            return FOD (direction);
          }



        protected:
          struct f_and_kpair {
            float f, k1, k2;
          };

          class PTF : public Eigen::Matrix<float, 4, 3>
          {
            public:
              using base_type = Eigen::Matrix<float, 4, 3>;
              using base_type::base_type;
              PTF (const Eigen::Vector3f&) = delete;

              const Eigen::Vector3f x()  const { return row(0).transpose(); }
              const Eigen::Vector3f T()  const { return row(1).transpose(); }
              const Eigen::Vector3f K1() const { return row(2).transpose(); }
              const Eigen::Vector3f K2() const { return row(3).transpose(); }

              void x  (const Eigen::Vector3f& value) { row(0) = value; }
              void T  (const Eigen::Vector3f& value) { row(1) = value; }
              void K1 (const Eigen::Vector3f& value) { row(2) = value; }
              void K2 (const Eigen::Vector3f& value) { row(3) = value; }
          };

          const Shared &S;
          Interpolator<Image<float>>::type source;
          std::uniform_real_distribution<> uniform_real;

          // With respect to the parallel transport frame,
          //   "pos" and "dir" already serve the purpose of "x" and "T";
          //   however we will store and track the PTF as a member variable, and just export "pos" and "dir" from it whenever necessary
          PTF F, F_seed;

          // TODO Once the iFOD2 approach of halving the contribution of the two segment endpoints is integrated,
          //   the values of K1 and K2 for the central vertices could theoretically be not calculated / discarded
          vector<PTF> probe;

          // TODO It seems that in their implementation, because they use a very small step size,
          //   they generate path probabilities based on multiple points within the arc,
          //   but ignore the sample points within the arc when generating the streamline.
          //   In the future, if we find a way to increase the step size, we may wish to use a
          //   similar system to iFOD2 that allows checking the intermediate vertices against
          //   any ROIs / 5TT image involved.

          float FOD (const Eigen::Vector3f &d) const
          {
            return (S.precomputer ? S.precomputer->value (values, d) : Math::SH::value (values, d, S.lmax));
          }

          float FOD (const PTF &frame)
          {
            if (!get_data (source, frame.x()))
              return NaN;
            return FOD (frame.T());
          }



          // TODO This serves a similar purpose to the iFOD* calibrator;
          //   may prefer to make use of the calibrator class in the future
          // TODO Would this be possible / applicable with the use of non-zero radius?
          template <bool seed>
          f_and_kpair rejection_sample()
          {
            float fmax = 0.0f;
            for (size_t i = 0; i != (seed ? S.max_trials_calibration_seeding : S.max_trials_calibration_tracking); ++i)
            {
              const f_and_kpair trial = seed ? random_init() : random_next();
              fmax = std::max (fmax, trial.f);
            }
            if (!fmax)
              return {NaN, NaN, NaN};
            fmax *= 2;
            for (size_t i = 0; i != (seed ? S.max_seed_attempts : S.max_trials_sampling); ++i)
            {
              const f_and_kpair trial = seed ? random_init() : random_next();
              if (uniform_real (rng) < (trial.f / fmax))
                return trial;
            }
            return {NaN, NaN, NaN};
          }



          f_and_kpair random_next()
          {
            float k1, k2;
            do {
              k1 = 2.0 * uniform_real(rng) - 1.0;
              k2 = 2.0 * uniform_real(rng) - 1.0;
            } while (Math::pow2(k1) + Math::pow2(k2) > 1);
            k1 /= S.kmax;
            k2 /= S.kmax;
            gen_probe (k1, k2);
            const float f = calc_support();
            return {f, k1, k2};
          }



          void gen_probe (const float k1, const float k2)
          {
            // TODO For now, we are not making any attempt at producing the cylindrical representation;
            //   just produce the line path
            // TODO Can the scale of computations be reduced here in order to only yield position & direction and ignore K#?
            Eigen::Matrix<float, 4, 4> propagator;
            probe[0] = F;
            for (size_t v = 1; v != S.nsamples; ++v)
            {
              propagator = P (k1, k2, S.probe_t[v]);
              probe[v].noalias() = propagator * F;
            }
          }

          float calc_support()
          {
            // TODO Just as for iFOD2, the FOD amplitudes at the start and end of each arc are
            //   shared between two arcs and therefore should only contribute half as much toward the
            //   path probabilities
            // Also, ideally they should not have to be recalculated, but should be carried over from the previous segment calculation
            // TODO May want to transition to sum of logs; especially if iFOD2-style sharing of first & last vertices is used
            //   (this would also facilitate dividing logs by number of samples, rather than taking an (nsamples)-th root)
            // TODO Trying sum of amplitudes rather than product;
            //   try contrasting the two, potentially even make command-line option
            float support = 0.0f;
            for (const auto &frame : probe) {
              const float amplitude = FOD (frame);
              if (amplitude < S.threshold)
                return 0.0f;
              support += amplitude;
            }
            support /= S.nsamples;
            return support;
          }

          f_and_kpair initialize (const Eigen::Vector3f& seed)
          {
            F.x (seed);
            return rejection_sample<true>();
          }

          f_and_kpair random_init()
          {
            F.x (pos);
            F.T (random_direction());
            F.K1 (random_direction());
            orthonormalize();
            const f_and_kpair trial = random_next();
            return trial;
          }

          void orthonormalize()
          {
            F.K2 (F.T().cross (F.K1()).normalized());
            F.K1 (F.K2().cross (F.T()).normalized());
          }



          Eigen::Matrix<float, 4, 4> P (const float k1, const float k2, const float t) const
          {
            const float kappa = std::sqrt (Math::pow2(k1) + Math::pow2(k2));
            const float kappa_sq = Math::pow2(kappa);
            const float kt = kappa * t;
            const float sin_kt = std::sin(kt);
            const float cos_kt = std::cos(kt);
            Eigen::Matrix<float, 4, 4> result;
            result << 1.0f, sin_kt / kappa,       k1 * (1.0f - cos_kt) / kappa_sq,                       k2 * (1.0f - cos_kt) / kappa_sq,
                      0.0f, cos_kt,               k1 * sin_kt / kappa,                                   k2 * sin_kt / kappa,
                      0.0f, -k1 * sin_kt / kappa, (Math::pow2(k2) + Math::pow2(k1) * cos_kt) / kappa_sq, k1 * k2 * (cos_kt - 1.0f) / kappa_sq,
                      0.0f, -k2 * sin_kt / kappa, k1 * k2 * (cos_kt - 1.0f) / kappa_sq,                  (Math::pow2(k1) + Math::pow2(k2) * cos_kt) / kappa_sq;
            return result;
          }
        };



      }
    }
  }
}

#endif
