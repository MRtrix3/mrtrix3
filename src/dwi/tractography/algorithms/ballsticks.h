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

#ifndef __dwi_tractography_algorithms_ballsticks_h__
#define __dwi_tractography_algorithms_ballsticks_h__

#include <set>

#include "image.h"
#include "algo/threaded_loop.h"
#include "interp/linear.h"

#include "dwi/tractography/bootstrap.h"
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


        enum class seed_fixel_t { FIRST, LARGEST, PROPORTIONAL, RANDOM_FIXEL, RANDOM_DIR };
        extern const char* const seed_fixel_options[];
        extern const App::OptionGroup BallSticksOptions;
        void load_ballsticks_options (Tractography::Properties&);



        class BallSticks : public MethodBase {
          public:
            static constexpr seed_fixel_t default_seed_fixel = seed_fixel_t::RANDOM_FIXEL;

            class Shared : public SharedBase {
              public:
                Shared (const std::string& diff_path, DWI::Tractography::Properties& property_set);
                size_t num_fixels;
                seed_fixel_t seed_fixel;
            };


            BallSticks (const Shared& shared) :
                MethodBase (shared),
                S (shared),
                uniform_real (0.0, 1.0),
                source (S.source) { }

            BallSticks (const BallSticks& that) :
                MethodBase (that.S),
                S (that.S),
                uniform_real (0.0, 1.0),
                source (S.source) { }


            bool init() override
            {
              source.clear();
              if (!source.get (pos, values))
                return false;

              if (S.init_dir.allFinite()) {
                const Fixel& fixel = nearest (S.init_dir);
                dir = fixel.dir;
                float dp = S.init_dir.dot (dir);
                if (dp < 0.0f) {
                  dir = -dir;
                  dp = -dp;
                }
                return (dir.allFinite() && fixel.f >= S.init_threshold && dp > S.cos_max_angle_1o);
              }

              switch (S.seed_fixel) {
                case seed_fixel_t::FIRST:
                {
                  dir = (*this)[0].dir;
                  return (dir.squaredNorm() && (*this)[0].f >= S.init_threshold);
                }
                case seed_fixel_t::LARGEST:
                {
                  float max_f = (*this)[0].f;
                  size_t max_index = 0;
                  for (size_t index = 1; index != S.num_fixels; ++index) {
                    if ((*this)[index].f > max_f) {
                      max_f = (*this)[index].f;
                      max_index = index;
                    }
                  }
                  dir = (*this)[max_index].dir;
                  return (dir.squaredNorm() && max_f >= S.init_threshold);
                }
                case seed_fixel_t::PROPORTIONAL:
                {
                  float sum_f = 0.0f;
                  for (size_t index = 0; index != S.num_fixels; ++index)
                    sum_f += (*this)[index].f;
                  const float select = uniform_real (rng) * sum_f;
                  float accumulator = 0.0f;
                  size_t index;
                  for (index = 0; index != S.num_fixels - 1; ++index) {
                    if ((accumulator += (*this)[index].f) > select)
                      break;
                  }
                  dir = (*this)[index].dir;
                  return (dir.squaredNorm() && (*this)[index].f >= S.init_threshold);
                }
                case seed_fixel_t::RANDOM_DIR:
                {
                  const Eigen::Vector3f d = random_direction();
                  const Fixel& fixel = nearest (d);
                  dir = fixel.dir;
                  return (dir.squaredNorm() && fixel.f >= S.init_threshold);
                }
                case seed_fixel_t::RANDOM_FIXEL:
                {
                  vector<size_t> suprathreshold_indices;
                  suprathreshold_indices.reserve (S.num_fixels);
                  for (size_t index = 0; index != S.num_fixels; ++index) {
                    if ((*this)[index].f >= S.init_threshold)
                      suprathreshold_indices.push_back (index);
                  }
                  switch (suprathreshold_indices.size()) {
                    case 0:
                      dir = Fixel::invalid.dir;
                      return false;
                    case 1:
                      dir = (*this)[suprathreshold_indices.front()].dir;
                      return (dir.squaredNorm() && (*this)[suprathreshold_indices.front()].f >= S.init_threshold);
                    default:
                    {
                      const size_t index = std::floor (uniform_real (rng) * suprathreshold_indices.size());
                      dir = (*this)[index].dir;
                      return (dir.squaredNorm() && (*this)[index].f >= S.init_threshold);
                    }
                  }
                }
              }
              assert (false);
              return false;
            }


            term_t next () override
            {
              if (!source.get (pos, values))
                return EXIT_IMAGE;
              const Fixel& fixel = nearest (dir);
              if (fixel.f < S.threshold)
                return MODEL;
              if (abs(dir.dot (fixel.dir)) < S.cos_max_angle_1o)
                return HIGH_CURVATURE;
              dir = dir.dot (fixel.dir) > 0.0 ? fixel.dir : -fixel.dir;
              pos += dir * S.step_size;
              return CONTINUE;
            }


            void truncate_track (GeneratedTrack& tck, const size_t length_to_revert_from, const size_t revert_step) override { assert (false); }


            float get_metric (const Eigen::Vector3f& position, const Eigen::Vector3f& direction)
            {
              if (!source.get (position, values))
                return 0.0;
              const Fixel& fixel = nearest (direction);
              return fixel.f;
            }


          protected:
            const Shared& S;
            // Used for proportional / random selection of fixel at seed point
            std::uniform_real_distribution<float> uniform_real;

            // Only derive from Interp::Linear in order to get the trilinear interpolation factors
            class Roulette : public Interp::Linear<BootstrapSample<Image<float>>> {
              public:
                Roulette (const Image<float>& bootstrap_vox) :
                    Interp::Linear<BootstrapSample<Image<float>>> (bootstrap_vox) { }

                bool get (const Eigen::Vector3f& pos, Eigen::VectorXf& data)
                {
                  if (!scanner (pos)) {
                    data.fill (NaN);
                    return false;
                  }

                  const float select = uniform_real (rng);
                  float accumulator = 0.0;
                  size_t i = 0;
                  for (ssize_t z = 0; z < 2; ++z) {
                    for (ssize_t y = 0; y < 2; ++y) {
                      for (ssize_t x = 0; x < 2; ++x) {
                        if (i == 7 || (accumulator += factors[i]) > select) {
                          index(2) = clamp (P[2]+z, size(2));
                          index(1) = clamp (P[1]+y, size(1));
                          index(0) = clamp (P[0]+x, size(0));
                          get_values (data);
                          return !std::isnan (data[0]);
                        }
                        ++i;
                      }
                    }
                  }

                  assert (false);
                  return false;
                }

              protected:
                std::uniform_real_distribution<float> uniform_real;

            } source;


#pragma pack(push, 1)
            struct Fixel {
              const Eigen::Vector3f dir;
              const float f;
              static const Fixel invalid;
            };
#pragma pack(pop)
            const Fixel& operator[] (const size_t index) const
            {
              assert (index <= S.num_fixels);
              if (index == S.num_fixels)
                return Fixel::invalid;
              return *reinterpret_cast<const Fixel* const>(&values[4*index]);
            }
            const Fixel& nearest (const Eigen::Vector3f& d) const
            {
              float max_dp = 0.0;
              size_t max_index = S.num_fixels;
              for (size_t i = 0; i != S.num_fixels; ++i) {
                float dp = std::abs (d.dot ((*this)[i].dir));
                if (dp > max_dp) {
                  max_dp = dp;
                  max_index = i;
                }
              }
              return (*this)[max_index];
            }

        };


        const BallSticks::Fixel BallSticks::Fixel::invalid {{NaN, NaN, NaN}, NaN};


      }
    }
  }
}



#endif



