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


#ifndef __registration_linear_h__
#define __registration_linear_h__

#include <vector>

#include "app.h"
#include "image.h"
#include "math/average_space.h"
#include "filter/normalise.h"
#include "filter/resize.h"
#include "filter/reslice.h"
#include "adapter/reslice.h"
#include "algo/threaded_loop.h"
#include "interp/linear.h"
#include "interp/nearest.h"
#include "registration/metric/params.h"
#include "registration/metric/normalised_cross_correlation.h"
#include "registration/metric/evaluate.h"
#include "registration/transform/initialiser.h"
#include "math/gradient_descent.h"
#include "math/gradient_descent_bb.h"
// #include "math/check_gradient.h"
#include "math/rng.h"
#include "math/math.h"
#include <iostream>
#include "registration/multi_resolution_lmax.h"
#include "registration/multi_contrast.h"

namespace MR
{
  namespace Registration
  {

    extern const App::OptionGroup adv_init_options;
    extern const App::OptionGroup lin_stage_options;
    extern const App::OptionGroup rigid_options;
    extern const App::OptionGroup affine_options;
    extern const App::OptionGroup fod_options;
    extern const char* optim_algo_names[];

    enum LinearMetricType {Diff, NCC};
    enum LinearRobustMetricEstimatorType {L1, L2, LP, None};
    enum OptimiserAlgoType {bbgd, gd, none};


    struct StageSetting {  MEMALIGN(StageSetting)
      StageSetting() :
        stage_iterations (1),
        gd_max_iter (500),
        scale_factor (1.0),
        optimisers (1, OptimiserAlgoType::bbgd),
        optimiser_default (OptimiserAlgoType::bbgd),
        optimiser_first (OptimiserAlgoType::bbgd),
        optimiser_last (OptimiserAlgoType::gd),
        loop_density (1.0),
        fod_lmax (-1) {}

      std::string info (const bool& do_reorientation = true) {
        std::string st;
        st = "scale factor " + str(scale_factor, 3);
        if (do_reorientation)
          st += ", lmax " + str(fod_lmax);
        st += ", GD max_iter " + str(gd_max_iter);
        if (loop_density < 1.0)
          st += ", GD density: " + str(loop_density);
        if (stage_iterations > 1)
          st += ", iterations: " + str(stage_iterations);
        st += ", optimiser: ";
        for (auto & optim : optimisers) {
          st += str(optim_algo_names[optim]) + " ";
        }
        return st;
      }
      size_t stage_iterations, gd_max_iter;
      default_type scale_factor;
      vector<OptimiserAlgoType> optimisers;
      OptimiserAlgoType optimiser_default, optimiser_first, optimiser_last;
      default_type loop_density;
      ssize_t fod_lmax;
      vector<std::string> diagnostics_images;
    } ;

    class Linear { MEMALIGN(Linear)

      public:

        Transform::Init::LinearInitialisationParams init;

        Linear () :
          stages (3),
          kernel_extent (3, 1),
          grad_tolerance (1.0e-6),
          step_tolerance (1.0e-10),
          log_stream (nullptr),
          init_translation_type (Transform::Init::mass),
          init_rotation_type (Transform::Init::none),
          robust_estimate (false),
          do_reorientation (false),
          //CONF option: reg_analyse_descent
          //CONF default: 0 (false)
          //CONF Linear registration: write comma separated gradient descent parameters and gradients
          //CONF to stdout and verbose gradient descent output to stderr
          analyse_descent (File::Config::get_bool ("reg_analyse_descent", false)) {
            stages[0].scale_factor = 0.25;
            stages[0].fod_lmax = 0;
            stages[1].scale_factor = 0.5;
            stages[1].fod_lmax = 2;
            stages[2].scale_factor = 1.0;
            stages[2].fod_lmax = 4;
        }

        // set_scale_factor needs to be the first option that is set as it overwrites the stage vector
        void set_scale_factor (const vector<default_type>& scalefactor) {
          stages.resize (scalefactor.size());
          for (size_t level = 0; level < scalefactor.size(); ++level) {
            if (scalefactor[level] <= 0 || scalefactor[level] > 1.0)
              throw Exception ("the linear registration scale factor for each multi-resolution level must be between 0 and 1");
            stages[level].scale_factor = scalefactor[level];
          }
        }

        // needs to be set before set_stage_iterations is set
        void set_stage_optimiser_default (const  OptimiserAlgoType& type) {
          for (auto & stage : stages)
            stage.optimiser_default = type;
        }

        // needs to be set before set_stage_iterations is set
        void set_stage_optimiser_first (const OptimiserAlgoType& type) {
          for (auto & stage : stages)
            stage.optimiser_first = type;
        }

        // needs to be set before set_stage_iterations is set
        void set_stage_optimiser_last (const OptimiserAlgoType& type) {
          for (auto & stage : stages)
            stage.optimiser_last = type;
        }

        void set_stage_iterations (const vector<int>& it) {
          for (size_t i = 0; i < it.size (); ++i)
            if (it[i] <= 0)
              throw Exception ("the number of stage iterations must be positive");
          if (it.size() == stages.size()) {
            for (size_t i = 0; i < stages.size (); ++i)
              stages[i].stage_iterations = it[i];
          } else if (it.size() == 1) {
            for (size_t i = 0; i < stages.size (); ++i)
              stages[i].stage_iterations = it[0];
          } else
              throw Exception ("the number of stage iterations must be defined for all stages (1 or " + str(stages.size())+")");
          for (auto & stage : stages) {
            stage.optimisers.resize(stage.stage_iterations, stage.optimiser_default);
            stage.optimisers[0] = stage.optimiser_first;
            if (stage.stage_iterations > 1)
            stage.optimisers[stage.stage_iterations - 1] = stage.optimiser_last;
          }
        }

        void set_max_iter (const vector<int>& maxiter) {
          for (size_t i = 0; i < maxiter.size (); ++i)
            if (maxiter[i] < 0)
              throw Exception ("the number of iterations must be non-negative");
          if (maxiter.size() == stages.size()) {
            for (size_t i = 0; i < stages.size (); ++i)
              stages[i].gd_max_iter = maxiter[i];
          } else if (maxiter.size() == 1) {
            for (size_t i = 0; i < stages.size (); ++i)
              stages[i].gd_max_iter = maxiter[0];
          } else
            throw Exception ("the number of gradient descent iterations must be defined for all stages (1 or " + str(stages.size())+")");
        }

        // needs to be set before set_lmax
        void set_directions (Eigen::MatrixXd& dir) {
          aPSF_directions = dir;
          do_reorientation = true;
        }

        void set_lmax (const vector<int>& lmax) {
          for (size_t i = 0; i < lmax.size (); ++i)
            if (lmax[i] < 0 || lmax[i] % 2)
              throw Exception ("the input lmax must be positive and even");
          if (lmax.size() == stages.size()) {
            for (size_t i = 0; i < stages.size (); ++i)
              stages[i].fod_lmax = lmax[i];
          } else if (lmax.size() == 1) {
            for (size_t i = 0; i < stages.size (); ++i)
              stages[i].fod_lmax = lmax[0];
          } else
            throw Exception ("the lmax must be defined for all stages (1 or " + str(stages.size())+")");
        }

        // needs to be set after set_lmax
        void set_mc_parameters (const vector<MultiContrastSetting>& mcs) {
          contrasts = mcs;
        }

        void set_loop_density (const vector<default_type>& loop_density_){
          for (size_t d = 0; d < loop_density_.size(); ++d)
            if (loop_density_[d] < 0.0 or loop_density_[d] > 1.0 )
              throw Exception ("loop density must be between 0.0 and 1.0");
          if (loop_density_.size() == stages.size()) {
            for (size_t i = 0; i < stages.size (); ++i)
              stages[i].loop_density = loop_density_[i];
          } else if (loop_density_.size() == 1) {
            for (size_t i = 0; i < stages.size (); ++i)
              stages[i].loop_density = loop_density_[0];
          } else
            throw Exception ("the lmax must be defined for all stages (1 or " + str(stages.size())+")");
        }

        void set_diagnostics_image_prefix (const std::basic_string<char>& diagnostics_image_prefix) {
          for (size_t level = 0; level < stages.size(); ++level) {
            auto & stage = stages[level];
            for (size_t iter = 1; iter <= stage.stage_iterations; ++iter) {
              std::ostringstream oss;
              oss << diagnostics_image_prefix << "_stage-" << level + 1 << "_iter-" << iter << ".mif";
              if (Path::exists(oss.str()) && !App::overwrite_files)
                throw Exception ("diagnostics image file \"" + oss.str() + "\" already exists (use -force option to force overwrite)");
              stage.diagnostics_images.push_back(oss.str());
            }
          }
        }

        void set_extent (const vector<size_t> extent) {
          for (size_t d = 0; d < extent.size(); ++d) {
            if (extent[d] < 1)
              throw Exception ("the neighborhood kernel extent must be at least 1 voxel");
          }
          kernel_extent = extent;
        }

        void set_init_translation_type (Transform::Init::InitType type) {
          init_translation_type = type;
        }

        void set_init_rotation_type (Transform::Init::InitType type) {
          init_rotation_type = type;
        }

        void use_robust_estimate (bool use) {
          robust_estimate = use;
        }


        void set_grad_tolerance (const float& tolerance) {
          grad_tolerance = tolerance;
        }

        void set_log_stream (std::streambuf* stream) {
          log_stream = stream;
        }

        ssize_t get_lmax () {
          ssize_t lmax=0;
          for (auto& s : stages)
            lmax = std::max(s.fod_lmax, lmax);
          return (int) lmax;
        }

        Header get_midway_header () {
          return Header(midway_image_header);
        }

        template <class MetricType, class TransformType, class Im1ImageType, class Im2ImageType>
        void run (
          MetricType& metric,
          TransformType& transform,
          Im1ImageType& im1_image,
          Im2ImageType& im2_image) {
            using BogusMaskType = Image<float>;
            run_masked<MetricType, TransformType, Im1ImageType, Im2ImageType, BogusMaskType, BogusMaskType >
              (metric, transform, im1_image, im2_image, nullptr, nullptr);
          }

        template <class MetricType, class TransformType, class Im1ImageType, class Im2ImageType, class Im2MaskType>
        void run_im2_mask (
          MetricType& metric,
          TransformType& transform,
          Im1ImageType& im1_image,
          Im2ImageType& im2_image,
          std::unique_ptr<Im2MaskType>& im2_mask) {
            using BogusMaskType = Image<float>;
            run_masked<MetricType, TransformType, Im1ImageType, Im2ImageType, BogusMaskType, Im2MaskType >
              (metric, transform, im1_image, im2_image, nullptr, im2_mask);
          }


        template <class MetricType, class TransformType, class Im1ImageType, class Im2ImageType, class Im1MaskType>
        void run_im1_mask (
          MetricType& metric,
          TransformType& transform,
          Im1ImageType& im1_image,
          Im2ImageType& im2_image,
          std::unique_ptr<Im1MaskType>& im1_mask) {
            using BogusMaskType = Image<float>;
            run_masked<MetricType, TransformType, Im1ImageType, Im2ImageType, Im1MaskType, BogusMaskType >
              (metric, transform, im1_image, im2_image, im1_mask, nullptr);
          }

        template <class MetricType, class TransformType, class Im1ImageType, class Im2ImageType, class Im1MaskType, class Im2MaskType>
        void run_masked (
          MetricType& metric,
          TransformType& transform,
          Im1ImageType& im1_image,
          Im2ImageType& im2_image,
          Im1MaskType& im1_mask,
          Im2MaskType& im2_mask) {

            if (do_reorientation)
              for (auto & s : stages)
                if (s.fod_lmax < 0)
                  throw Exception ("the lmax needs to be defined for each registration stage");

            if (init_translation_type == Transform::Init::mass)
              Transform::Init::initialise_using_image_mass (im1_image, im2_image, im1_mask, im2_mask, transform, init, contrasts);
            else if (init_translation_type == Transform::Init::geometric)
              Transform::Init::initialise_using_image_centres (im1_image, im2_image, im1_mask, im2_mask, transform, init);
            else if (init_translation_type == Transform::Init::set_centre_mass) // doesn't change translation or linear matrix
              Transform::Init::set_centre_via_mass (im1_image, im2_image, im1_mask, im2_mask, transform, init, contrasts);
            else if (init_translation_type == Transform::Init::set_centre_geometric) // doesn't change translation or linear matrix
              Transform::Init::set_centre_via_image_centres (im1_image, im2_image, im1_mask, im2_mask, transform, init);

            if (init_rotation_type == Transform::Init::moments)
              Transform::Init::initialise_using_image_moments (im1_image, im2_image, im1_mask, im2_mask, transform, init, contrasts);
            else if (init_rotation_type == Transform::Init::rot_search)
              Transform::Init::initialise_using_rotation_search (im1_image, im2_image, im1_mask, im2_mask, transform, init, contrasts);


            INFO ("Transformation before registration:");
            INFO (transform.info());

            INFO ("Linear registration stage parameters:");
            for (auto & stage : stages) {
              INFO(stage.info());
            }

            // TODO global search
            // if (global_search) {
            //   GlobalSearch::GlobalSearch transformation_search;
            //   if (log_stream) {
            //     transformation_search.set_log_stream(log_stream);
            //   }
            //   // std::ofstream outputFile( "/tmp/log.txt" );
            //   // transformation_search.set_log_stream(outputFile.rdbuf());
            //   // transformation_search.set_log_stream(std::cerr.rdbuf());
            //   transformation_search.run_masked (metric, transform, im1_image, im2_image, im1_mask, im2_mask, init);
            //   // transform.debug();
            // }

            using MidwayImageType = Header;
            using ProcessedImageType = Im1ImageType;
            using ProcessedMaskType = Image<bool>;

#ifdef REGISTRATION_CUBIC_INTERP
            // using Im1ImageInterpolatorType = Interp::SplineInterp<Im1ImageType, Math::UniformBSpline<typename Im1ImageType::value_type>, Math::SplineProcessingType::ValueAndDerivative>;
            // using Im2ImageInterpolatorType = Interp::SplineInterp<Im2ImageType, Math::UniformBSpline<typename Im2ImageType::value_type>, Math::SplineProcessingType::ValueAndDerivative>;
            // using ProcessedImageInterpolatorType = Interp::SplineInterp<ProcessedImageType, Math::UniformBSpline<typename ProcessedImageType::value_type>, Math::SplineProcessingType::ValueAndDerivative>;
#else
            using Im1ImageInterpolatorType = Interp::LinearInterp<Im1ImageType, Interp::LinearInterpProcessingType::ValueAndDerivative>;
            using Im2ImageInterpolatorType = Interp::LinearInterp<Im2ImageType, Interp::LinearInterpProcessingType::ValueAndDerivative>;
            using ProcessedImageInterpolatorType = Interp::LinearInterp<ProcessedImageType, Interp::LinearInterpProcessingType::ValueAndDerivative>;
#endif

            using ParamType = Metric::Params<TransformType,
                                   Im1ImageType,
                                   Im2ImageType,
                                   MidwayImageType,
                                   Im1MaskType,
                                   Im2MaskType,
                                   Im1ImageInterpolatorType,
                                   Im2ImageInterpolatorType,
                                   Interp::Linear<Im1MaskType>,
                                   Interp::Linear<Im2MaskType>,
                                   ProcessedImageType,
                                   ProcessedImageInterpolatorType,
                                   ProcessedMaskType,
                                   Interp::Nearest<ProcessedMaskType>>;

            Eigen::Matrix<typename TransformType::ParameterType, Eigen::Dynamic, 1> optimiser_weights = transform.get_optimiser_weights();

            // calculate midway (affine average) space which will be constant for each resolution level
            midway_image_header = compute_minimum_average_header (im1_image, im2_image, transform.get_transform_half_inverse(), transform.get_transform_half());

            for (size_t istage = 0; istage < stages.size(); istage++) {
              auto& stage = stages[istage];

              CONSOLE ("linear stage " + str(istage + 1) + "/"+str(stages.size()) + ", " + stage.info(do_reorientation));
              // define or adjust tissue contrast lmax, nvols for this stage
              stage_contrasts = contrasts;
              if (stage_contrasts.size()) {
                for (auto & mc : stage_contrasts) {
                  mc.lower_lmax (stage.fod_lmax);
                }
              } else {
                MultiContrastSetting mc (im1_image.ndim()<4? 1:im1_image.size(3), do_reorientation, stage.fod_lmax);
                stage_contrasts.push_back(mc);
              }

              DEBUG ("before downsampling:");
              for (const auto & mc : stage_contrasts)
                DEBUG (str(mc));

              INFO ("smoothing image 1");
              auto im1_smoothed = Registration::multi_resolution_lmax (im1_image, stage.scale_factor, do_reorientation, stage_contrasts);
              INFO ("smoothing image 2");
              auto im2_smoothed = Registration::multi_resolution_lmax (im2_image, stage.scale_factor, do_reorientation, stage_contrasts, &stage_contrasts);

              DEBUG ("after downsampling:");
              for (const auto & mc : stage_contrasts)
                INFO (str(mc));


              Filter::Resize midway_resize_filter (midway_image_header);
              midway_resize_filter.set_scale_factor (stage.scale_factor);
              Header midway_resized (midway_resize_filter);

              ParamType parameters (transform, im1_smoothed, im2_smoothed, midway_resized, im1_mask, im2_mask);
              parameters.loop_density = stage.loop_density;
              if (contrasts.size())
                parameters.set_mc_settings (stage_contrasts);


              // if (robust_estimate)
              //   INFO ("using robust estimate");
              // parameters.robust_estimate = robust_estimate; // TODO

              // set control point coordinates inside +-1/3 of the midway_image size
              {
                Eigen::Vector3 ext (midway_image_header.spacing(0) / 6.0,
                                    midway_image_header.spacing(1) / 6.0,
                                    midway_image_header.spacing(2) / 6.0);
                for (size_t i = 0; i<3; ++i)
                  ext(i) *= midway_image_header.size(i) - 0.5;
                parameters.set_control_points_extent(ext);
              }
              DEBUG ("neighbourhood kernel extent: " + str(kernel_extent));
              parameters.set_extent (kernel_extent);
              Eigen::Vector3d spacing (
                midway_image_header.spacing(0),
                midway_image_header.spacing(1),
                midway_image_header.spacing(2));
              Eigen::Vector3d coherence(spacing);
              Eigen::Vector3d stop(spacing);
              //CONF option: reg_coherence_len
              //CONF default: 3.0
              //CONF Linear registration: estimated spatial coherence length in voxels
              default_type reg_coherence_len = File::Config::get_float ("reg_coherence_len", 3.0); // = 3 stdev blur
              coherence *= reg_coherence_len * 1.0 / (2.0 * stage.scale_factor);
              //CONF option: reg_stop_len
              //CONF default: 0.0001
              //CONF Linear registration: smallest gradient descent step measured in fraction of a voxel at which to stop registration
              default_type reg_stop_len = File::Config::get_float ("reg_stop_len", 0.0001);
              stop.array() *= reg_stop_len;
              DEBUG ("coherence length: " + str(coherence));
              DEBUG ("stop length:      " + str(stop));
              transform.get_gradient_descent_updator()->set_control_points (parameters.control_points, coherence, stop, spacing);

              // convergence check using slope of smoothed parameter trajectories
              Eigen::VectorXd slope_threshold = Eigen::VectorXd::Ones (12);
              //CONF option: reg_gd_convergence_thresh
              //CONF default: 5e-3
              //CONF Linear registration: threshold for convergence check using the smoothed control point trajectories
              //CONF measured in fraction of a voxel
              slope_threshold.fill (spacing.mean() * File::Config::get_float ("reg_gd_convergence_thresh", 5e-3f));
              DEBUG ("convergence slope threshold: " + str(slope_threshold[0]));
              //CONF option: reg_gd_convergence_data_smooth
              //CONF default: 0.8
              //CONF Linear registration: control point trajectory smoothing value used in convergence check
              //CONF parameter range: [0...1]
              const default_type alpha (MR::File::Config::get_float ("reg_gd_convergence_data_smooth", 0.8));
              if ( (alpha < 0.0f ) || (alpha > 1.0f) )
                throw Exception ("config file option reg_gd_convergence_data_smooth has to be in the range: [0...1]");
              //CONF option: reg_gd_convergence_slope_smooth
              //CONF default: 0.1
              //CONF Linear registration: control point trajectory slope smoothing value used in convergence check
              //CONF parameter range: [0...1]
              const default_type beta (MR::File::Config::get_float ("reg_gd_convergence_slope_smooth", 0.1));
              if ( (beta < 0.0f ) || (beta > 1.0f) )
                throw Exception ("config file option reg_gd_convergence_slope_smooth has to be in the range: [0...1]");
              size_t buffer_len (MR::File::Config::get_float ("reg_gd_convergence_buffer_len", 4));
              //CONF option: reg_gd_convergence_min_iter
              //CONF default: 10
              //CONF Linear registration: minimum number of iterations until convergence check is activated
              size_t min_iter (MR::File::Config::get_float ("reg_gd_convergence_min_iter", 10));
              transform.get_gradient_descent_updator()->set_convergence_check (slope_threshold, alpha, beta, buffer_len, min_iter);

              Metric::Evaluate<MetricType, ParamType> evaluate (metric, parameters);
              if (do_reorientation && stage.fod_lmax > 0)
                evaluate.set_directions (aPSF_directions);

              INFO ("registration stage running...");
              for (auto stage_iter = 1U; stage_iter <= stage.stage_iterations; ++stage_iter) {
                if (stage.gd_max_iter > 0 and stage.optimisers[stage_iter - 1] == OptimiserAlgoType::bbgd) {
                  Math::GradientDescentBB<Metric::Evaluate<MetricType, ParamType>, typename TransformType::UpdateType>
                  optim (evaluate, *transform.get_gradient_descent_updator());
                  optim.be_verbose (analyse_descent);
                  optim.precondition (optimiser_weights);
                  optim.run (stage.gd_max_iter, grad_tolerance, analyse_descent ? std::cout.rdbuf() : log_stream);
                  parameters.optimiser_update (optim, evaluate.overlap());
                  INFO ("    iteration: "+str(stage_iter)+"/"+str(stage.stage_iterations)+" GD iterations: "+
                  str(optim.function_evaluations())+" cost: "+str(optim.value())+" overlap: "+str(evaluate.overlap()));
                } else if (stage.gd_max_iter > 0) {
                  Math::GradientDescent<Metric::Evaluate<MetricType, ParamType>, typename TransformType::UpdateType>
                    optim (evaluate, *transform.get_gradient_descent_updator());
                  optim.be_verbose (analyse_descent);
                  optim.precondition (optimiser_weights);
                  optim.run (stage.gd_max_iter, grad_tolerance, analyse_descent ? std::cout.rdbuf() : log_stream);
                  parameters.optimiser_update (optim, evaluate.overlap());
                  INFO ("    iteration: "+str(stage_iter)+"/"+str(stage.stage_iterations)+" GD iterations: "+
                  str(optim.function_evaluations())+" cost: "+str(optim.value())+" overlap: "+str(evaluate.overlap()));
                }

                if (log_stream) {
                  std::ostream log_os(log_stream);
                  log_os << "\n\n"; // two empty lines for gnuplot's index recognition
                }
                // debug cost function gradient
                // auto params = optim.state();
                // VAR(optim.function_evaluations());
                // Math::check_function_gradient (evaluate, params, 0.0001, true, optimiser_weights);
                if (stage.diagnostics_images.size()) {
                  CONSOLE("    creating diagnostics image: " + stage.diagnostics_images[stage_iter - 1]);
                  parameters.make_diagnostics_image (stage.diagnostics_images[stage_iter - 1], File::Config::get_bool ("reg_linreg_diagnostics_image_masked", false));
                }
              }
              // update midway (affine average) space using the current transformations
              midway_image_header = compute_minimum_average_header (im1_image, im2_image, parameters.transformation.get_transform_half_inverse(), parameters.transformation.get_transform_half());
            }
            INFO("linear registration done");
            INFO (transform.info());
          }

          // template<class ImageType, class TransformType>
          // void transform_image_midway (const ImageType& input, const TransformType& transformation,
          //   const bool do_reorientation, const bool input_is_one, const std::string& out_path, const Header& h_midway) {
          //   if (do_reorientation and aPSF_directions.size() == 0)
          //     throw Exception ("directions have to be calculated before reorientation");

          //   Image<typename ImageType::value_type> image_midway;
          //   Header midway_header (h_midway);
          //   midway_header.ndim() = input.ndim();
          //   for (size_t dim = 3; dim < input.ndim(); ++dim) {
          //     midway_header.spacing(dim) = input.spacing(dim);
          //     midway_header.size(dim) = input.size(dim);
          //   }
          //   image_midway = Image<typename ImageType::value_type>::create (out_path, midway_header).with_direct_io();
          //   if (input_is_one) {
          //     Filter::reslice<Interp::Cubic> (input, image_midway, transformation.get_transform_half(), Adapter::AutoOverSample, 0.0);
          //     if (do_reorientation)
          //  //     60 directions by default!
          //       Transform::reorient ("reorienting...", image_midway, image_midway, transformation.get_transform_half(), aPSF_directions);
          //   } else {
          //     Filter::reslice<Interp::Cubic> (input, image_midway, transformation.get_transform_half_inverse(), Adapter::AutoOverSample, 0.0);
          //     if (do_reorientation)
          //  //     60 directions by default!
          //       Transform::reorient ("reorienting...", image_midway, image_midway, transformation.get_transform_half_inverse(), aPSF_directions);
          //   }
          // }

      protected:
        vector<StageSetting> stages;
        vector<MultiContrastSetting> contrasts, stage_contrasts;
        vector<size_t> kernel_extent;
        default_type grad_tolerance;
        default_type step_tolerance;
        std::streambuf* log_stream;
        Transform::Init::InitType init_translation_type, init_rotation_type;
        bool robust_estimate;
        bool do_reorientation;
        Eigen::MatrixXd aPSF_directions;
        const bool analyse_descent;

        Header midway_image_header;
    };

    void set_init_translation_model_from_option (Registration::Linear& registration, const int& option);
    void set_init_rotation_model_from_option (Registration::Linear& registration, const int& option);
    void parse_general_options (Registration::Linear& registration);
  }
}

#endif
