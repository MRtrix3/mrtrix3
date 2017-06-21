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

    void calc_median_trafo (const MR::vector<transform_type>& candid_trafo, const Eigen::Matrix<default_type, 4, 4> & vertices_4,
      const transform_type & trafo_before, transform_type & median_trafo, MR::vector<default_type> & scores);

    template <typename ParamType, typename StageType, typename MetricType, typename TransformType>
    struct RobustStage {

      RobustStage (ParamType & params, const StageType & stage, MetricType & metric, bool do_reorientation,
      const Eigen::MatrixXd & aPSF_directions, const Eigen::Matrix<typename TransformType::ParameterType, Eigen::Dynamic, 1> & optimiser_weights,
      const default_type& grad_tolerance, bool analyse_descent = false, std::streambuf* log_stream = nullptr) {
        params.processed_mask = ParamType::ProcessedMaskType::scratch (Header(params.midway_image));
        params.processed_image = ParamType::ProcessedImageType::scratch (Header(params.midway_image));
        for (auto l = Loop (0, 3) (params.processed_image); l; ++l) {
          params.processed_image.value() = NaN;
        }
        using ParameterType = typename TransformType::ParameterType;
        // using TransformType = decltype(parameters.transformation);
        using UpdateType = typename TransformType::UpdateType;

        ///////////////////////////////// parameters ///////////////////////////////////////
        MR::vector<int> ntiles (3, File::Config::get_int ("reg_linreg_robust_ntiles", 8));
        int max_tilesize = File::Config::get_int ("reg_linreg_robust_max_tilesize", 15);
        int robust_maxiter = File::Config::get_int ("reg_linreg_robust_maxiter", 20);
        default_type mask_fraction = File::Config::get_float ("reg_linreg_robust_mask_fraction", 0.15);
          // store current parameters
        Eigen::Matrix<ParameterType, Eigen::Dynamic, 1> x_before;
        params.transformation.get_parameter_vector(x_before);
        const transform_type trafo_before = params.transformation.get_transform_half();
        default_type score_thresh = File::Config::get_float ("reg_linreg_robust_score_thresh", 0.5);
        bool force_maxiter = File::Config::get_bool ("reg_linreg_robust_force_maxiter", false);
        /////////////////////////////////            ///////////////////////////////////////

        const int n_vertices = 4;
        const Eigen::Matrix<default_type, 4, n_vertices> vertices_4 = params.control_points.template cast<default_type> ();

        bool converged = false;
        int patchwidth_x = std::min<int>(max_tilesize, std::floor<int>((float) params.midway_image.size(0) / ntiles[0]));
        int patchwidth_y = std::min<int>(max_tilesize, std::floor<int>((float) params.midway_image.size(1) / ntiles[1]));
        int patchwidth_z = std::min<int>(max_tilesize, std::floor<int>((float) params.midway_image.size(2) / ntiles[2]));
        ntiles[0] = std::ceil<int>(float(params.midway_image.size(0)) / patchwidth_x);
        ntiles[0] = std::ceil<int>(float(params.midway_image.size(1)) / patchwidth_y);
        ntiles[0] = std::ceil<int>(float(params.midway_image.size(2)) / patchwidth_z);
        // 25% of the volume needs to be filled with both images
        ssize_t minoverlap = std::floor<ssize_t> ( params.loop_density * mask_fraction * patchwidth_x * patchwidth_y * patchwidth_z);
        int robust_gditer = 0;

        // transformation for each patch
        while (!converged) {
          ProgressBar progress ("robust estimate in "+str(ntiles[0]*ntiles[1]*ntiles[2])+" VOIs of size "+str(patchwidth_x)+"x"+str(patchwidth_y)+"x"+str(patchwidth_z), ntiles[0]*ntiles[1]*ntiles[2]);
          ParamType parameters (params);
          MR::vector<transform_type> candid_trafo;
          MR::vector<MR::vector<int>> candid_pos, candid_size;
          MR::vector<ssize_t> candid_overlap;

          parameters.robust_estimate_subset_from.resize(3);
          parameters.robust_estimate_subset_size.resize(3);

          for (int zpatch = 0; zpatch < ntiles[2]; zpatch++) {
            for (int ypatch = 0; ypatch < ntiles[1]; ypatch++) {
              for (int xpatch = 0; xpatch < ntiles[0]; xpatch++) {
                parameters.transformation.set_parameter_vector(x_before);
                parameters.robust_estimate_subset_from[0] = patchwidth_x * xpatch;
                parameters.robust_estimate_subset_from[1] = patchwidth_y * ypatch;
                parameters.robust_estimate_subset_from[2] = patchwidth_z * zpatch;
                parameters.robust_estimate_subset_size[0] = std::min<int>(patchwidth_x, parameters.midway_image.size(0) - parameters.robust_estimate_subset_from[0] - 1);
                parameters.robust_estimate_subset_size[1] = std::min<int>(patchwidth_y, parameters.midway_image.size(1) - parameters.robust_estimate_subset_from[1] - 1);
                parameters.robust_estimate_subset_size[2] = std::min<int>(patchwidth_z, parameters.midway_image.size(2) - parameters.robust_estimate_subset_from[2] - 1);

                Metric::Evaluate<MetricType, ParamType> evaluate (metric, parameters);
                if (do_reorientation && stage.fod_lmax > 0)
                  evaluate.set_directions (aPSF_directions);
                Math::GradientDescentBB<Metric::Evaluate<MetricType, ParamType>, UpdateType>
                  optim (evaluate, *parameters.transformation.get_gradient_descent_updator());
                optim.be_verbose (analyse_descent);
                optim.precondition (optimiser_weights);
                optim.run (robust_maxiter, grad_tolerance, log_stream);
                minoverlap = std::floor<ssize_t> ( params.loop_density * mask_fraction *
                 parameters.robust_estimate_subset_size[0] * parameters.robust_estimate_subset_size[1] * parameters.robust_estimate_subset_size[2]);
                if (is_finite(optim.state()) and evaluate.overlap() > minoverlap) {
                  transform_type T;
                  Transform::param_vec2mat(optim.state().template cast <default_type> (), T.matrix());
                  candid_trafo.push_back(T);
                  candid_overlap.push_back(evaluate.overlap());
                  candid_pos.push_back(parameters.robust_estimate_subset_from);
                  candid_size.push_back(parameters.robust_estimate_subset_size);
                }
                DEBUG ("    robust iteration "+str(parameters.robust_estimate_subset_from)+" GD iterations: "+str(robust_gditer) +" cost: "+str(optim.value())+" overlap: "+str(evaluate.overlap())+" "+str(optim.function_evaluations()));
                progress++;
              }
            }
          }
          robust_gditer += robust_maxiter;
          INFO ("    robust GD iterations: "+str(robust_gditer));
          if (candid_pos.size() < 3) {
            throw Exception ("require more than two valid regions to compute robust update. got: " +str(candid_pos.size()));
          }

          transform_type median_trafo;
          MR::vector<default_type> scores(candid_trafo.size());
          // calculate median transformation
          calc_median_trafo (candid_trafo, vertices_4, trafo_before, median_trafo, scores);

          size_t trusted_voxels = 0;
          for (size_t iroi = 0; iroi < candid_trafo.size(); iroi++) {
            Adapter::Subset<decltype(params.processed_image)> subset (params.processed_image, candid_pos[iroi], candid_size[iroi]);
            Adapter::Subset<decltype(params.processed_mask)> subsetmask (params.processed_mask, candid_pos[iroi], candid_size[iroi]);
            if (scores[iroi] < 0.5)
              trusted_voxels += candid_size[iroi][0] * candid_size[iroi][1] * candid_size[iroi][2];
            for (auto l = Loop (0, 3) (subset, subsetmask); l; ++l) {
              subset.value() = scores[iroi];
              subsetmask.value() = scores[iroi] < 0.5;
            }
          }
          if (!trusted_voxels) {
            WARN ("no robust consensus found. using best VOI");
            size_t iroi = std::distance(scores.begin(), std::min_element(scores.begin(), scores.end()));
            Adapter::Subset<decltype(params.processed_image)> subset (params.processed_image, candid_pos[iroi], candid_size[iroi]);
            Adapter::Subset<decltype(params.processed_mask)> subsetmask (params.processed_mask, candid_pos[iroi], candid_size[iroi]);
            if (scores[iroi] < 0.5)
              trusted_voxels += candid_size[iroi][0] * candid_size[iroi][1] * candid_size[iroi][2];
            for (auto l = Loop (0, 3) (subset, subsetmask); l; ++l) {
              subset.value() = scores[iroi];
              subsetmask.value() = scores[iroi] < score_thresh;
            }
          }
          INFO ("    selected voxels: " + str(trusted_voxels) + " / " + str(params.midway_image.size(0) * params.midway_image.size(1) * params.midway_image.size(2)));
          INFO ("    creating score images");
          {
            Header h1 = Header(params.im1_image);
            Header h2 = Header(params.im2_image);
            h1.ndim() = 3;
            h2.ndim() = 3;
            params.robust_estimate_score1 = Image<float>::scratch(h1);
            params.robust_estimate_score2 = Image<float>::scratch(h2);
            params.robust_estimate_score1_interp.reset (new Interp::Linear<Image<float>> (params.robust_estimate_score1));
            params.robust_estimate_score2_interp.reset (new Interp::Linear<Image<float>> (params.robust_estimate_score2));

            vector<int> no_oversampling (3,1);
            Adapter::Reslice<Interp::Linear, decltype(params.processed_image)> score_reslicer1 (
              params.processed_image, params.robust_estimate_score1, params.transformation.get_transform_half().inverse(), no_oversampling, NAN);
            for (auto i = Loop (0, 3) (params.robust_estimate_score1, score_reslicer1); i; ++i) {
              params.robust_estimate_score1.value() = 1.0 - score_reslicer1.value();
            }
            Adapter::Reslice<Interp::Linear, decltype(params.processed_image)> score_reslicer2 (
              params.processed_image, params.robust_estimate_score2, params.transformation.get_transform_half_inverse().inverse(), no_oversampling, NAN);
            for (auto i = Loop (0, 3) (params.robust_estimate_score2, score_reslicer2); i; ++i) {
              params.robust_estimate_score2.value() = 1.0 - score_reslicer2.value();
            }
          }

          //   MR::Transform T (params.robust_estimate_score1);
          //   for (auto l = Loop (0, 3) (params.robust_estimate_score1); l; ++l) {
          //     Eigen::Vector3 voxel_pos ((default_type)params.robust_estimate_score1.index(0), (default_type)params.robust_estimate_score1.index(1), (default_type)params.robust_estimate_score1.index(2));
          //     Eigen::Vector3 coord = T.voxel2scanner * voxel_pos;

          //     params.robust_estimate_score1.value() =
          //   }
          // }
          INFO ("    registering masked images");
          params.robust_estimate_subset = false;
          params.robust_estimate_use_score = true;
          Metric::Evaluate<MetricType, ParamType> evaluate (metric, params);
          if (do_reorientation && stage.fod_lmax > 0)
            evaluate.set_directions (aPSF_directions);
          Math::GradientDescentBB<Metric::Evaluate<MetricType, ParamType>, UpdateType>
            optim (evaluate, *params.transformation.get_gradient_descent_updator());
          optim.be_verbose (analyse_descent);
          optim.precondition (optimiser_weights);

          int maxiter(force_maxiter ? robust_maxiter : stage.gd_max_iter);
          optim.run (maxiter , grad_tolerance, log_stream);
          INFO ("    GD iterations: "+str(optim.function_evaluations())+" cost: "+str(optim.value())+" overlap: "+str(evaluate.overlap()));
          params.optimiser_update (optim, evaluate.overlap());

          converged = !force_maxiter || robust_gditer >= stage.gd_max_iter || (optim.function_evaluations() - 2) < maxiter; // - 2 because of BBGD initialisation
          params.robust_estimate_subset = true;
          params.robust_estimate_use_score = false;
        }
        params.robust_estimate_subset = false;
        params.robust_estimate_use_score = false;
      }
    };
  }
}

#endif
