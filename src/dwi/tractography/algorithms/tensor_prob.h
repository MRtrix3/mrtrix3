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

        class Tensor_Prob : public Tensor_Det {
          public:
            class Shared : public Tensor_Det::Shared {
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

            class WildBootstrap {
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
                    residuals[i] = std::exp (-residuals[i]) - data[i];
                    data[i] += uniform_int (*rng) ? residuals[i] : -residuals[i];
                  }
                }

              private:
                const Eigen::MatrixXf& H;
                std::uniform_int_distribution<> uniform_int;
                Eigen::VectorXf residuals, log_signal;
            };


            class Interp : public Interpolator<Bootstrap<Image<float>,WildBootstrap>>::type {
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

                  auto add_values = [&](float fraction, int sig_index) {
                    get_values (raw_signals[sig_index]);
                    data += fraction * raw_signals[sig_index];
                  };

                  if (factors[0]) add_values (factors[0], 0);
                  ++index(2);
                  if (factors[1]) add_values (factors[1], 1);
                  ++index(1);
                  if (factors[2]) add_values (factors[2], 3);
                  --index(2);
                  if (factors[3]) add_values (factors[3], 2);
                  ++index(0);
                  if (factors[4]) add_values (factors[4], 6);
                  --index(1);
                  if (factors[5]) add_values (factors[5], 4);
                  ++index(2);
                  if (factors[6]) add_values (factors[6], 5);
                  ++index(1);
                  if (factors[7]) add_values (factors[7], 7);
                  --index(0);
                  --index(1);
                  --index(2);

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



