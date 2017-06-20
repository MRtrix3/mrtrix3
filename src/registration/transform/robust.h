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

#ifndef __registration_transform_robust_h__
#define __registration_transform_robust_h__
#include "types.h"
#include <iostream>
#include <vector>
#include <iostream>
#include "app.h"
#include "image.h"
#include "math/average_space.h"
// #include "algo/threaded_loop.h"
#include "registration/metric/params.h"
#include "registration/metric/evaluate.h"
// #include "math/gradient_descent.h"
#include "math/gradient_descent_bb.h"
// #include "math/rng.h"
#include "math/math.h"
#include "registration/multi_contrast.h"
#include "registration/transform/affine.h"

namespace MR
{
  namespace Registration
  {
    // median_weiszfeld(const MatrixType& X, VectorType& median, const size_t numIter = 300, const default_type precision = 0.00001)
    template <typename ParamType, typename StageType, typename MetricType, typename TransformType>
    void robust_stage (ParamType & params, const StageType & stage, MetricType & metric, bool do_reorientation,
      const Eigen::MatrixXd & aPSF_directions, const Eigen::Matrix<typename TransformType::ParameterType, Eigen::Dynamic, 1> & optimiser_weights,
      const default_type& grad_tolerance, bool analyse_descent = false, std::streambuf* log_stream = nullptr) {
      ParamType parameters (params);
      using TransformParamType = typename TransformType::ParameterType;
      // using TransformType = decltype(parameters.transformation);
      using UpdateType = typename TransformType::UpdateType;

      MR::vector<int> ntiles (3, 5);
      int robust_maxiter = 5;
      const int n_vertices = 4;


      ntiles[0] = std::min<int>(ntiles[0], parameters.midway_image.size(0));
      ntiles[1] = std::min<int>(ntiles[1], parameters.midway_image.size(1));
      ntiles[2] = std::min<int>(ntiles[2], parameters.midway_image.size(2));
      bool converged = false;
      int patchwidth_x = std::ceil<int>((float) parameters.midway_image.size(0) / ntiles[0]);
      int patchwidth_y = std::ceil<int>((float) parameters.midway_image.size(1) / ntiles[1]);
      int patchwidth_z = std::ceil<int>((float) parameters.midway_image.size(2) / ntiles[2]);
      // 25% of the volume needs to be filled with both images
      ssize_t minoverlap = std::floor<ssize_t> ( parameters.loop_density * 0.25 * patchwidth_x * patchwidth_y * patchwidth_z);
      int robust_gditer = 0;
        // store current parameters
      Eigen::Matrix<TransformParamType, Eigen::Dynamic, 1> x_before;
      parameters.transformation.get_parameter_vector(x_before);
      const Eigen::Matrix<TransformParamType, 4, n_vertices> vertices_4 = parameters.control_points;
      MAT(vertices_4);

      INFO ("    robust estimate in "+str(ntiles)+" tiles of size "+str(patchwidth_x)+"x"+str(patchwidth_y)+"x"+str(patchwidth_z));
      while (!converged) {
        MR::vector<Eigen::Matrix<TransformParamType, Eigen::Dynamic, 1>> candid_x;
        MR::vector<default_type> candid_cost;
        MR::vector<MR::vector<int>> candid_pos;
        MR::vector<int> candid_niter;
        MR::vector<ssize_t> candid_overlap;
        MR::vector<bool> candid_converged;
        parameters.robust_from.resize(3);
        parameters.robust_size.resize(3);

        for (int zpatch = 0; zpatch < ntiles[2]; zpatch++) {
          for (int ypatch = 0; ypatch < ntiles[1]; ypatch++) {
            for (int xpatch = 0; xpatch < ntiles[0]; xpatch++) {
              parameters.transformation.set_parameter_vector(x_before);
              parameters.robust_from[0] = patchwidth_x * xpatch;
              parameters.robust_from[1] = patchwidth_y * ypatch;
              parameters.robust_from[2] = patchwidth_z * zpatch;
              parameters.robust_size[0] = std::min<int>(patchwidth_x, parameters.midway_image.size(0) - parameters.robust_from[0]);
              parameters.robust_size[1] = std::min<int>(patchwidth_y, parameters.midway_image.size(1) - parameters.robust_from[1]);
              parameters.robust_size[2] = std::min<int>(patchwidth_z, parameters.midway_image.size(2) - parameters.robust_from[2]);

              Metric::Evaluate<MetricType, ParamType> evaluate (metric, parameters);
              if (do_reorientation && stage.fod_lmax > 0)
                evaluate.set_directions (aPSF_directions);
              Math::GradientDescentBB<Metric::Evaluate<MetricType, ParamType>, UpdateType>
                optim (evaluate, *parameters.transformation.get_gradient_descent_updator());
              optim.be_verbose (analyse_descent);
              optim.precondition (optimiser_weights);
              optim.run (robust_maxiter, grad_tolerance, log_stream);
              if (is_finite(optim.state()) and evaluate.overlap() >= minoverlap) {
                candid_x.push_back(optim.state());
                candid_cost.push_back(optim.value());
                candid_overlap.push_back(evaluate.overlap());
                candid_pos.emplace_back(std::initializer_list<int>{xpatch, ypatch, zpatch});
                candid_converged.push_back(x_before.isApprox(optim.state()) or optim.function_evaluations() < robust_maxiter);
                candid_niter.push_back(optim.function_evaluations());
              }
              DEBUG ("    robust iteration "+str(parameters.robust_from)+" GD iterations: "+str(robust_gditer) +" cost: "+str(optim.value())+" overlap: "+str(evaluate.overlap())+" "+str(optim.function_evaluations()));
            }
          }
        }
        robust_gditer += robust_maxiter;
        // calculate consensus

        // parameters.processed_mask
        // consesus_overlap
        // consesus_x

        // update parameters with consensus
        // parameters.optimiser_update (consensus_x, consesus_overlap);

        // TODO check convergence
        converged = robust_gditer >= stage.gd_max_iter;
      }
      INFO ("    GD iterations: "+str(robust_gditer));
    }
  }
}

#endif
