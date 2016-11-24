/*
 * Copyright (c) 2008-2016 the MRtrix3 contributors
 * 
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/
 * 
 * MRtrix is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * 
 * For more details, see www.mrtrix.org
 * 
 */

#ifndef __dwi_tractography_algorithms_tensor_prob_h__
#define __dwi_tractography_algorithms_tensor_prob_h__

#include "dwi/bootstrap.h"
#include "dwi/tractography/algorithms/tensor_det.h"
#include "dwi/tractography/rng.h"


namespace MR
{
  namespace DWI
  {
    namespace Tractography
    {
      namespace Algorithms
      {

        using namespace MR::DWI::Tractography::Tracking;

        class Tensor_Prob : public Tensor_Det { MEMALIGN(Tensor_Prob)
          public:
            class Shared : public Tensor_Det::Shared { MEMALIGN(Shared)
              public:
                Shared (const std::string& diff_path, DWI::Tractography::Properties& property_set) :
                  Tensor_Det::Shared (diff_path, property_set) {

                    if (is_act() && act().backtrack())
                      throw Exception ("Sorry, backtracking not currently enabled for TensorProb algorithm");

                    properties["method"] = "TensorProb";

                    Hat = bmat * binv;
                  }

                Eigen::MatrixXf Hat;
            };






            Tensor_Prob (const Shared& shared) :
              Tensor_Det (shared),
              S (shared),
              source (Bootstrap<Image<float>,WildBootstrap> (S.source, WildBootstrap (S.Hat))) { }

            Tensor_Prob (const Tensor_Prob& F) :
              Tensor_Det (F.S),
              S (F.S),
              source (Bootstrap<Image<float>,WildBootstrap> (S.source, WildBootstrap (S.Hat))) { }



            bool init() {
              source.clear();
              if (!source.get (pos, values))
                return false;
              return Tensor_Det::do_init();
            }



            term_t next () {
              if (!source.get (pos, values))
                return EXIT_IMAGE;
              return Tensor_Det::do_next();
            }


            void truncate_track (std::vector<Eigen::Vector3f>& tck, const size_t length_to_revert_from, const int revert_step) {}


          protected:

            class WildBootstrap { MEMALIGN(WildBootstrap)
              public:
                WildBootstrap (const Eigen::MatrixXf& hat_matrix) :
                  H (hat_matrix),
                  uniform_int (0, 1),
                  residuals (H.rows()),
                  log_signal (H.rows()) { }

                void operator() (float* data) {
                  for (ssize_t i = 0; i < residuals.size(); ++i)
                    log_signal[i] = data[i] > float (0.0) ? -std::log (data[i]) : float (0.0);

                  residuals = H * log_signal;

                  for (ssize_t i = 0; i < residuals.size(); ++i) {
                    residuals[i] = residuals[i] ? (data[i] - std::exp (-residuals[i])) : float(0.0);
                    data[i] += uniform_int (*rng) ? residuals[i] : -residuals[i];
                  }
                }

              private:
                const Eigen::MatrixXf& H;
                std::uniform_int_distribution<> uniform_int;
                Eigen::VectorXf residuals, log_signal;
            };


            class Interp : public Interpolator<Bootstrap<Image<float>,WildBootstrap>>::type { MEMALIGN(Interp)
              public:
                Interp (const Bootstrap<Image<float>,WildBootstrap>& bootstrap_vox) :
                  Interpolator<Bootstrap<Image<float>,WildBootstrap> >::type (bootstrap_vox) {
                    for (size_t i = 0; i < 8; ++i)
                      raw_signals.push_back (Eigen::VectorXf (size(3)));
                  }

                std::vector<Eigen::VectorXf> raw_signals;

                bool get (const Eigen::Vector3f& pos, Eigen::VectorXf& data) {
                  scanner (pos);
                  if (out_of_bounds) {
                    data.fill (NaN);
                    return false;
                  }

                  data.setZero();

                  // Modified to be consistent with the new Interp::Linear implementation
                  size_t i = 0;
                  for (ssize_t z = 0; z < 2; ++z) {
                    index(2) = clamp (P[2]+z, size(2));
                    for (ssize_t y = 0; y < 2; ++y) {
                      index(1) = clamp (P[1]+y, size(1));
                      for (ssize_t x = 0; x < 2; ++x) {
                        index(0) = clamp (P[0]+x, size(0));
                        if (factors[i]) {
                          get_values (raw_signals[i]);
                          data += factors[i] * raw_signals[i];
                        }
                        ++i;
                      }
                    }
                  }

                  return !std::isnan (data[0]);
                }
            };

            const Shared& S;
            Interp source;


        };

      }
    }
  }
}

#endif



