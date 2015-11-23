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


            void truncate_track (std::vector<Eigen::Vector3f>& tck, const int revert_step) {}


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
                    //for (ssize_t i = 0; i < data.size(); ++i)
                      //data[i] += fraction * raw_signals[sig_index][i];
                  };

                  if (faaa) 
                    add_values (faaa, 0);

                  ++index(2);
                  if (faab) 
                    add_values (faab, 1);

                  ++index(1);
                  if (fabb) 
                    add_values (fabb, 3);

                  --index(2);
                  if (faba) 
                    add_values (faba, 2);

                  ++index(0);
                  if (fbba) 
                    add_values (fbba, 6);

                  --index(1);
                  if (fbaa) 
                    add_values (fbaa, 4);

                  ++index(2);
                  if (fbab) 
                    add_values (fbab, 5);

                  ++index(1);
                  if (fbbb) 
                    add_values (fbbb, 7);

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



