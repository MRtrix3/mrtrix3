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

namespace MR
{
  namespace Registration
  {

    extern const App::OptionGroup adv_init_options;
    extern const App::OptionGroup rigid_options;
    extern const App::OptionGroup affine_options;
    extern const App::OptionGroup fod_options;

    enum LinearMetricType {Diff, NCC};
    enum LinearRobustMetricEstimatorType {L1, L2, LP, None};

    class Linear
    {

      public:

        Transform::Init::LinearInitialisationParams init;

        Linear () :
          max_iter (1, 500),
          gd_repetitions (1, 1),
          scale_factor (3),
          loop_density (1, 1.0),
          kernel_extent(3, 1),
          grad_tolerance(1.0e-6),
          step_tolerance(1.0e-10),
          log_stream (nullptr),
          init_translation_type (Transform::Init::mass),
          init_rotation_type (Transform::Init::none),
          robust_estimate (false),
          do_reorientation (false),
          fod_lmax (3),
          reg_bbgd (File::Config::get_bool ("reg_bbgd", true)),
          analyse_descent (File::Config::get_bool ("reg_analyse_descent", false)) {
          scale_factor[0] = 0.25;
          scale_factor[1] = 0.5;
          scale_factor[2] = 1.0;
          fod_lmax[0] = 0;
          fod_lmax[1] = 2;
          fod_lmax[2] = 4;
        }

        void set_max_iter (const std::vector<int>& maxiter) {
          for (size_t i = 0; i < maxiter.size (); ++i)
            if (maxiter[i] < 0)
              throw Exception ("the number of iterations must be positive");
          max_iter = maxiter;
        }

        void set_gradient_descent_repetitions (const std::vector<int>& rep) {
          for (size_t i = 0; i < rep.size (); ++i)
            if (rep[i] < 0)
              throw Exception ("the number of repetitions must be positive");
          gd_repetitions = rep;
        }

        void set_scale_factor (const std::vector<default_type>& scalefactor) {
          for (size_t level = 0; level < scalefactor.size(); ++level) {
            if (scalefactor[level] <= 0 || scalefactor[level] > 1)
              throw Exception ("the linear registration scale factor for each multi-resolution level must be between 0 and 1");
          }
          scale_factor = scalefactor;
        }

        void set_extent (const std::vector<size_t> extent) {
          for (size_t d = 0; d < extent.size(); ++d) {
            if (extent[d] < 1)
              throw Exception ("the neighborhood kernel extent must be at least 1 voxel");
          }
          kernel_extent = extent;
        }

        void set_loop_density (const std::vector<default_type>& loop_density_){
          for (size_t d = 0; d < loop_density_.size(); ++d)
            if (loop_density_[d] < 0.0 or loop_density_[d] > 1.0 )
              throw Exception ("loop density must be between 0.0 and 1.0");
          loop_density = loop_density_;
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

        void set_directions (Eigen::MatrixXd& dir) {
          aPSF_directions = dir;
          do_reorientation = true;
        }

        void set_grad_tolerance (const float& tolerance) {
          grad_tolerance = tolerance;
        }

        void set_log_stream (std::streambuf* stream) {
          log_stream = stream;
        }

        void set_lmax (const std::vector<int>& lmax) {
          for (size_t i = 0; i < lmax.size (); ++i)
            if (lmax[i] < 0 || lmax[i] % 2)
              throw Exception ("the input rigid and affine lmax must be positive and even");
          fod_lmax = lmax;
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
            typedef Image<float> BogusMaskType;
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
            typedef Image<float> BogusMaskType;
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
            typedef Image<float> BogusMaskType;
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

            if (max_iter.size() == 1)
              max_iter.resize (scale_factor.size(), max_iter[0]);
            else if (max_iter.size() != scale_factor.size())
              throw Exception ("the max number of iterations needs to be defined for each multi-resolution level (scale factor)");

            if (do_reorientation and (fod_lmax.size() != scale_factor.size()))
              throw Exception ("the lmax needs to be defined for each multi-resolution level (scale factor)");
            else
              fod_lmax.resize (scale_factor.size(), 0);

            if (gd_repetitions.size() == 1)
              gd_repetitions.resize (scale_factor.size(), gd_repetitions[0]);
            else if (gd_repetitions.size() != scale_factor.size())
              throw Exception ("the number of gradient descent repetitions needs to be defined for each multi-resolution level");

            if (loop_density.size() == 1)
              loop_density.resize (scale_factor.size(), loop_density[0]);
            else if (loop_density.size() != scale_factor.size())
              throw Exception ("the loop density level needs to be defined for each multi-resolution level");

            if (init_translation_type == Transform::Init::mass)
              Transform::Init::initialise_using_image_mass (im1_image, im2_image, im1_mask, im2_mask, transform, init);
            else if (init_translation_type == Transform::Init::geometric)
              Transform::Init::initialise_using_image_centres (im1_image, im2_image, im1_mask, im2_mask, transform, init);
            else if (init_translation_type == Transform::Init::set_centre_mass) // doesn't change translation or linear matrix
              Transform::Init::set_centre_via_mass (im1_image, im2_image, im1_mask, im2_mask, transform, init);
            else if (init_translation_type == Transform::Init::set_centre_geometric) // doesn't change translation or linear matrix
              Transform::Init::set_centre_via_image_centres (im1_image, im2_image, im1_mask, im2_mask, transform, init);

            if (init_rotation_type == Transform::Init::moments)
              Transform::Init::initialise_using_image_moments (im1_image, im2_image, im1_mask, im2_mask, transform, init);
            else if (init_rotation_type == Transform::Init::rot_search) {
              Transform::Init::initialise_using_rotation_search (
                im1_image, im2_image, im1_mask, im2_mask, transform, init);
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

            typedef Header MidwayImageType;
            typedef Im1ImageType ProcessedImageType;
            typedef Image<bool> ProcessedMaskType;

#ifdef REGISTRATION_CUBIC_INTERP
            // typedef Interp::SplineInterp<Im1ImageType, Math::UniformBSpline<typename Im1ImageType::value_type>, Math::SplineProcessingType::ValueAndDerivative> Im1ImageInterpolatorType;
            // typedef Interp::SplineInterp<Im2ImageType, Math::UniformBSpline<typename Im2ImageType::value_type>, Math::SplineProcessingType::ValueAndDerivative> Im2ImageInterpolatorType;
            // typedef Interp::SplineInterp<ProcessedImageType, Math::UniformBSpline<typename ProcessedImageType::value_type>, Math::SplineProcessingType::ValueAndDerivative> ProcessedImageInterpolatorType;
#else
            typedef Interp::LinearInterp<Im1ImageType, Interp::LinearInterpProcessingType::ValueAndDerivative> Im1ImageInterpolatorType;
            typedef Interp::LinearInterp<Im2ImageType, Interp::LinearInterpProcessingType::ValueAndDerivative> Im2ImageInterpolatorType;
            typedef Interp::LinearInterp<ProcessedImageType, Interp::LinearInterpProcessingType::ValueAndDerivative> ProcessedImageInterpolatorType;
#endif

            typedef Metric::Params<TransformType,
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
                                   Interp::Nearest<ProcessedMaskType>> ParamType;

            Eigen::Matrix<typename TransformType::ParameterType, Eigen::Dynamic, 1> optimiser_weights = transform.get_optimiser_weights();
            const Eigen::Matrix<default_type, 4, 1> midspace_padding = Eigen::Matrix<default_type, 4, 1>(1.0, 1.0, 1.0, 1.0);
            const int midspace_voxel_subsampling = 1;

            // calculate midway (affine average) space which will be constant for each resolution level
            midway_image_header = compute_minimum_average_header (im1_image, im2_image, transform, midspace_voxel_subsampling, midspace_padding);

            for (size_t level = 0; level < scale_factor.size(); level++) {
              {
                std::string st;
                loop_density[level] < 1.0 ? st = ", loop density: " + str(loop_density[level]) : st = "";
                if (do_reorientation) {
                  CONSOLE ("multi-resolution level " + str(level + 1) + ", scale factor " + str(scale_factor[level]) + st + ", lmax " + str(fod_lmax[level]) );
                } else {
                  CONSOLE ("multi-resolution level " + str(level + 1) + ", scale factor " + str(scale_factor[level]) + st);
                }
              }

              auto im1_smoothed = Registration::multi_resolution_lmax (im1_image, scale_factor[level], do_reorientation, fod_lmax[level]);
              auto im2_smoothed = Registration::multi_resolution_lmax (im2_image, scale_factor[level], do_reorientation, fod_lmax[level]);

              Filter::Resize midway_resize_filter (midway_image_header);
              midway_resize_filter.set_scale_factor (scale_factor[level]);
              Header midway_resized (midway_resize_filter);

              ParamType parameters (transform, im1_smoothed, im2_smoothed, midway_resized, im1_mask, im2_mask);
              INFO ("loop density: " + str(loop_density[level]));
              parameters.loop_density = loop_density[level];
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
              default_type reg_coherence_len = File::Config::get_float ("reg_coherence_len", 3.0); // = 3 stdev blur
              coherence *= reg_coherence_len * 1.0 / (2.0 * scale_factor[level]);
              default_type reg_stop_len = File::Config::get_float ("reg_stop_len", 0.0001);
              stop.array() *= reg_stop_len;
              DEBUG ("coherence length: " + str(coherence));
              DEBUG ("stop length:      " + str(stop));
              transform.get_gradient_descent_updator()->set_control_points(
                parameters.control_points, coherence, stop, spacing);

              Metric::Evaluate<MetricType, ParamType> evaluate (metric, parameters);
              if (do_reorientation && fod_lmax[level] > 0)
                evaluate.set_directions (aPSF_directions);


              for (auto gd_iteration = 0; gd_iteration < gd_repetitions[level]; ++gd_iteration){
                if (max_iter[level] == 0)
                  continue;
                if (reg_bbgd) {
                  Math::GradientDescentBB<Metric::Evaluate<MetricType, ParamType>, typename TransformType::UpdateType>
                    optim (evaluate, *transform.get_gradient_descent_updator());
                  optim.be_verbose (analyse_descent);
                  optim.precondition (optimiser_weights);
                  if (analyse_descent)
                    optim.run (max_iter[level], grad_tolerance, std::cout.rdbuf());
                  else
                    optim.run (max_iter[level], grad_tolerance);
                  DEBUG ("gradient descent ran using " + str(optim.function_evaluations()) + " cost function evaluations.");
                  if (!is_finite(optim.state())) {
                    throw Exception ("registration failed: encountered NaN in parameters.");
                  }
                  parameters.transformation.set_parameter_vector (optim.state());
                  parameters.update_control_points();
                } else {
                  Math::GradientDescent<Metric::Evaluate<MetricType, ParamType>, typename TransformType::UpdateType>
                    optim (evaluate, *transform.get_gradient_descent_updator());
                  optim.be_verbose (analyse_descent);
                  optim.precondition (optimiser_weights);
                  if (analyse_descent)
                    optim.run (max_iter[level], grad_tolerance, std::cout.rdbuf());
                  else
                    optim.run (max_iter[level], grad_tolerance);
                  DEBUG ("gradient descent ran using " + str(optim.function_evaluations()) + " cost function evaluations.");
                  if (!is_finite(optim.state())) {
                    throw Exception ("registration failed due to NaN in parameters");
                  }
                  parameters.transformation.set_parameter_vector (optim.state());
                  parameters.update_control_points();
                }
                if (log_stream){
                  std::ostream log_os(log_stream);
                  log_os << "\n\n"; // two empty lines for gnuplot's index recognition
                }
                // auto params = optim.state();
                // VAR(optim.function_evaluations());
                // Math::check_function_gradient (evaluate, params, 0.0001, true, optimiser_weights);
              }
              // update midway (affine average) space using the current transformations
              midway_image_header = compute_minimum_average_header(im1_image, im2_image, parameters.transformation, midspace_voxel_subsampling, midspace_padding);
            }
          }

        template<class Im1ImageType, class Im2ImageType, class TransformType>
        void write_transformed_images (
          const Im1ImageType& im1_image,
          const Im2ImageType& im2_image,
          const TransformType& transformation,
          const std::string& im1_path,
          const std::string& im2_path,
          const bool do_reorientation) {

            if (do_reorientation and aPSF_directions.size() == 0)
              throw Exception ("directions have to be calculated before reorientation");

            Image<typename Im1ImageType::value_type> image1_midway;
            Image<typename Im2ImageType::value_type> image2_midway;

            Header image1_midway_header (midway_image_header);
            image1_midway_header.datatype() = DataType::Float64;
            image1_midway_header.ndim() = im1_image.ndim();
            for (size_t dim = 3; dim < im1_image.ndim(); ++dim){
              image1_midway_header.spacing(dim) = im1_image.spacing(dim);
              image1_midway_header.size(dim) = im1_image.size(dim);
            }
            image1_midway = Image<typename Im1ImageType::value_type>::create (im1_path, image1_midway_header).with_direct_io();
            Header image2_midway_header (midway_image_header);
            image2_midway_header.datatype() = DataType::Float64;
            image2_midway_header.ndim() = im2_image.ndim();
            for (size_t dim = 3; dim < im2_image.ndim(); ++dim){
              image2_midway_header.spacing(dim) = im2_image.spacing(dim);
              image2_midway_header.size(dim) = im2_image.size(dim);
            }
            image2_midway = Image<typename Im2ImageType::value_type>::create (im2_path, image2_midway_header).with_direct_io();

            Filter::reslice<Interp::Cubic> (im1_image, image1_midway, transformation.get_transform_half(), Adapter::AutoOverSample, 0.0);
            Filter::reslice<Interp::Cubic> (im2_image, image2_midway, transformation.get_transform_half_inverse(), Adapter::AutoOverSample, 0.0);
            if (do_reorientation){
              Transform::reorient ("reorienting...", image1_midway, image1_midway, transformation.get_transform_half(), aPSF_directions);
              Transform::reorient ("reorienting...", image2_midway, image2_midway, transformation.get_transform_half_inverse(), aPSF_directions);
            }
          }

      protected:
        std::vector<int> max_iter;
        std::vector<int> gd_repetitions;
        std::vector<default_type> scale_factor;
        std::vector<default_type> loop_density;
        std::vector<size_t> kernel_extent;
        default_type grad_tolerance;
        default_type step_tolerance;
        std::streambuf* log_stream;
        Transform::Init::InitType init_translation_type, init_rotation_type;
        bool robust_estimate;
        bool do_reorientation;
        Eigen::MatrixXd aPSF_directions;
        std::vector<int> fod_lmax;
        const bool reg_bbgd, analyse_descent;

        Header midway_image_header;
    };

    void set_init_translation_model_from_option (Registration::Linear& registration, const int& option);
    void set_init_rotation_model_from_option (Registration::Linear& registration, const int& option);
    void parse_general_init_options (Registration::Linear& registration);
  }
}

#endif
