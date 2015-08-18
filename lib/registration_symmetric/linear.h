///*
// Copyright 2012 Brain Research Institute, Melbourne, Australia

// Written by David Raffelt, 24/02/2012

// This file is part of MRtrix.

// MRtrix is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

// MRtrix is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with MRtrix.  If not, see <http://www.gnu.org/licenses/>.

// */

//#ifndef __registration_symmetric_linear_h__
//#define __registration_symmetric_linear_h__

//#include <vector>

//#include "image/voxel.h"
//#include "image/buffer_scratch.h"
//#include "image/filter/resize.h"
//#include "image/interp/linear.h"
//#include "image/interp/nearest.h"

//#include "image/registration_symmetric/metric/params.h"
//#include "image/registration_symmetric/metric/evaluate.h"
//#include "image/registration_symmetric/transform/initialiser.h"

//#include "math/matrix.h"
//#include "math/gradient_descent.h"
//#include "math/check_gradient.h"
//#include "math/rng.h"

//// #include "eigen/matrix.h" // matrix_average
//// #include "eigen/gsl_eigen.h" // gslMatrix2eigenMatrix
//// #include <Eigen/Geometry> // Eigen::Translation
//#include "image/average_space.h"
//#include "eigen/gsl_eigen.h"

//#include <iostream>     // std::streambuf
//#include <image/iterator.h>


//namespace MR
//{
//  namespace Image
//  {
//    namespace RegistrationSymmetric
//    {

//      extern const App::OptionGroup rigid_options;
//      extern const App::OptionGroup affine_options;
//      extern const App::OptionGroup syn_options;
//      extern const App::OptionGroup initialisation_options;
//      extern const App::OptionGroup fod_options;



//      class Linear
//      {

//        public:

//          Linear () :
//            max_iter (1, 300),
//            scale_factor (2),
//            kernel_extent(3),
//            grad_tolerance(1.0e-6),
//            step_tolerance(1.0e-10),
//            relative_cost_improvement_tolerance(1.0e-10),
//            gradient_criterion_tolerance(1.0e-6),
//            log_stream(nullptr),
//            init_type (Transform::Init::mass) {
//            scale_factor[0] = 0.5;
//            scale_factor[1] = 1;
//            kernel_extent[0] = 1;
//            kernel_extent[1] = 1;
//            kernel_extent[2] = 1;
//          }

//          void set_max_iter (const std::vector<int>& maxiter) {
//            for (size_t i = 0; i < maxiter.size (); ++i)
//              if (maxiter[i] < 0)
//                throw Exception ("the number of iterations must be positive");
//            max_iter = maxiter;
//          }

//          void set_scale_factor (const std::vector<float>& scalefactor) {
//            for (size_t level = 0; level < scalefactor.size(); ++level) {
//              if (scalefactor[level] <= 0 || scalefactor[level] > 1)
//                throw Exception ("the scale factor for each multi-resolution level must be between 0 and 1");
//            }
//            scale_factor = scalefactor;
//          }

//          void set_extent (const std::vector<size_t> extent) {
//			  // TODO: check input
//        	  for (size_t d = 0; d < extent.size(); ++d) {
//        		  if (extent[d] < 1)
//					  throw Exception ("the neighborhood kernel extent must be at least 1 voxel");
//        	  }
//        	  kernel_extent = extent;
//			}

////          void get_extent (std::vector<size_t>& extent_vector) const {
////            for (size_t i = 0; i < 3; ++i) {
////            	extent_vector[i] = this->kernel_extent[i];
////            }
////          }

//          void set_init_type (Transform::Init::InitType type) {
//            init_type = type;
//          }

//          void set_transform_type (Transform::Init::InitType type) {
//            init_type = type;
//          }

//          void set_directions (Math::Matrix<float>& dir) {
//            directions = dir;
//          }

//          void set_grad_tolerance (const float& tolerance) {
//            grad_tolerance = tolerance;
//          }

//          void set_step_tolerance (const float& tolerance) {
//            step_tolerance = tolerance;
//          }

//          void set_relative_cost_improvement_tolerance (const float& tolerance) {
//            relative_cost_improvement_tolerance = tolerance;
//          }

//          void set_gradient_criterion_tolerance (const float& tolerance) {
//            gradient_criterion_tolerance = tolerance;
//          }

//          void set_gradient_descent_log_stream (std::streambuf* stream) {
//            log_stream = stream;
//          }

//          template <class MetricType, class TransformType, class MovingVoxelType, class TemplateVoxelType>
//          void run (
//            MetricType& metric,
//            TransformType& transform, //
//            MovingVoxelType& moving_vox,
//            TemplateVoxelType& template_vox) {
//              typedef Image::BufferScratch<bool>::voxel_type BogusMaskType;
//              run_masked<MetricType, TransformType, MovingVoxelType, TemplateVoxelType, BogusMaskType, BogusMaskType >
//                (metric, transform, moving_vox, template_vox, nullptr, nullptr);
//            }

//          template <class MetricType, class TransformType, class MovingVoxelType, class TemplateVoxelType, class TemplateMaskType>
//          void run_template_mask (
//            MetricType& metric,
//            TransformType& transform,
//            MovingVoxelType& moving_vox,
//            TemplateVoxelType& template_vox,
//            std::unique_ptr<TemplateMaskType>& template_mask) {
//              typedef Image::BufferScratch<bool>::voxel_type BogusMaskType;
//              run_masked<MetricType, TransformType, MovingVoxelType, TemplateVoxelType, BogusMaskType, TemplateMaskType >
//                (metric, transform, moving_vox, template_vox, nullptr, template_mask);
//            }


//          template <class MetricType, class TransformType, class MovingVoxelType, class TemplateVoxelType, class MovingMaskType>
//          void run_moving_mask (
//            MetricType& metric,
//            TransformType& transform,
//            MovingVoxelType& moving_vox,
//            TemplateVoxelType& template_vox,
//            std::unique_ptr<MovingMaskType>& moving_mask) {
//              typedef Image::BufferScratch<bool>::voxel_type BogusMaskType;
//              run_masked<MetricType, TransformType, MovingVoxelType, TemplateVoxelType, MovingMaskType, BogusMaskType >
//                (metric, transform, moving_vox, template_vox, moving_mask, nullptr);
//            }

//          template <class MetricType, class TransformType, class MovingVoxelType, class TemplateVoxelType, class MovingMaskType, class TemplateMaskType>
//          void run_masked (
//            MetricType& metric,
//            TransformType& transform,
//            MovingVoxelType& moving_vox,
//            TemplateVoxelType& template_vox,
//            std::unique_ptr<MovingMaskType>& moving_mask,
//            std::unique_ptr<TemplateMaskType>& template_mask) {
//              if (max_iter.size() == 1)
//                max_iter.resize (scale_factor.size(), max_iter[0]);
//              else if (max_iter.size() != scale_factor.size())
//                throw Exception ("the max number of iterations needs to be defined for each multi-resolution level");

//              if (init_type == Transform::Init::mass)
//                Transform::Init::initialise_using_image_mass (moving_vox, template_vox, transform);
//              else if (init_type == Transform::Init::geometric)
//                Transform::Init::initialise_using_image_centres (moving_vox, template_vox, transform);

//              {
//                // Math::Matrix<double> mat_half;
//                // transform.get_transform_half(mat_half);
//                // std::cerr << mat_half << std::endl;

//                // Math::Matrix<double> mat;
//                // transform.get_transform(mat);
//                // std::cerr << mat << std::endl;

//                // Math::Matrix<double> mat_too;
//                // mat_too.allocate(4,4);
//                // for (int row = 0; row != 4; ++row) {
//                //   for (int col = 0; col != 4; ++col) {
//                //     double sum = 0;
//                //     for (int inner = 0; inner != 4; ++inner){
//                //       sum += mat_half(row,inner) * mat_half(inner,col);
//                //     }
//                //     mat_too(row,col) = sum;
//                //   }
//                // }
//                // std::cerr << mat_too << std::endl;

//                // Math::Matrix<double> mat_half_inverse;
//                // transform.get_transform_half_inverse(mat_half_inverse);
//                // std::cerr << mat_half_inverse << std::endl;
//              }

//              typedef typename MovingMaskType::voxel_type MovingMaskVoxelType;
//              typedef typename TemplateMaskType::voxel_type TemplateMaskVoxelType;

//              typedef Image::BufferScratch<float> BufferType;
//              typedef Image::Interp::Linear<BufferType::voxel_type > MovingImageInterpolatorType;
//              typedef Image::Interp::Linear<BufferType::voxel_type > TemplateImageInterpolatorType;

//              typedef BufferType::voxel_type MidWayVoxelType;

//              typedef Metric::Params<TransformType,
//                                     BufferType::voxel_type,
//                                     BufferType::voxel_type,
//                                     MidWayVoxelType,
//                                     MovingImageInterpolatorType,
//                                     TemplateImageInterpolatorType,
//                                     Image::Interp::Nearest<MovingMaskVoxelType >,
//                                     Image::Interp::Nearest<TemplateMaskVoxelType > > ParamType;

//              Math::Vector<typename TransformType::ParameterType> optimiser_weights;
//              transform.get_optimiser_weights (optimiser_weights);

//              for (size_t level = 0; level < scale_factor.size(); level++) {

//                CONSOLE ("multi-resolution level " + str(level + 1) + ", scale factor: " + str(scale_factor[level]));

//                Image::Filter::Resize moving_resize_filter (moving_vox);
//                moving_resize_filter.set_scale_factor (scale_factor[level]);
//                moving_resize_filter.set_interp_type (1);
//                std::shared_ptr<BufferType> moving_resized_ptr(new BufferType (moving_resize_filter.info()));
//                // BufferType moving_resized (moving_resize_filter.info());
//                BufferType::voxel_type moving_resized_vox (*moving_resized_ptr);
//                Image::Filter::Smooth moving_smooth_filter (moving_resized_vox);

//                BufferType moving_resized_smoothed (moving_smooth_filter.info());
//                BufferType::voxel_type moving_resized_smoothed_vox (moving_resized_smoothed);

//                Image::Filter::Resize template_resize_filter (template_vox);
//                template_resize_filter.set_scale_factor (scale_factor[level]);
//                template_resize_filter.set_interp_type (1);
//                std::shared_ptr<BufferType> template_resized_ptr(new BufferType (template_resize_filter.info()));
//                // BufferType template_resized (template_resize_filter.info());
//                BufferType::voxel_type template_resized_vox (*template_resized_ptr);
//                Image::Filter::Smooth template_smooth_filter (template_resized_vox);
//                BufferType template_resized_smoothed (template_smooth_filter.info());
//                BufferType::voxel_type template_resized_smoothed_vox (template_resized_smoothed);


//                std::vector<std::shared_ptr<BufferType> >  input_images;
//                input_images.push_back (template_resized_ptr);
//                input_images.push_back (moving_resized_ptr);
                
//                double voxel_subsampling = 1.0;
//                Eigen::Matrix<double, 4, 1>  padding;
//                padding << 1.0, 1.0, 1.0, 1.0;
//                std::vector<Eigen::Transform< double, 3, Eigen::Projective>> transform_header_with; // TODO

//                // define transfomations that will be applied to the image header when the common space is calculated
//                {
//                  Eigen::Transform<double, 3, Eigen::Projective> init_trafo_t;
//                  Eigen::Transform<double, 3, Eigen::Projective> init_trafo_m;
//                  Math::Matrix<double> mat_half;
//                  Math::Matrix<double> mat_half_inverse;
//                  transform.get_transform_half (mat_half);
//                  transform.get_transform_half_inverse (mat_half_inverse);
//                  Eigen::gslMatrix2eigenMatrix<Math::Matrix<double>, decltype(init_trafo_t.matrix())> (mat_half,init_trafo_t.matrix());
//                  Eigen::gslMatrix2eigenMatrix<Math::Matrix<double>, decltype(init_trafo_m.matrix())> (mat_half_inverse,init_trafo_m.matrix());
//                  transform_header_with.push_back(init_trafo_t);
//                  transform_header_with.push_back(init_trafo_m);
//                  // Eigen::Translation (const VectorType &vector)
//                }

//                // auto midway_info = moving_resized_ptr->info();
//                INFO("computing midspace");
//                auto midway_info = compute_minimum_average_info<double,std::shared_ptr<BufferType>>(input_images, voxel_subsampling, padding, transform_header_with);
//                BufferType midway_image (midway_info);
//                MidWayVoxelType midway_vox (midway_image);
                

//                DEBUG( "moving_info:   \n" + str(moving_resized_ptr->transform()) + "\n" + str(moving_resized_ptr->info()));
//                DEBUG( "template_info: \n" + str(template_resized_ptr->transform()) + "\n" + str(template_resized_ptr->info()));
//                DEBUG( "midway_info:   \n" + str(midway_info.transform()) + "\n" + str(midway_info));

//                {
//                  LogLevelLatch log_level (0);
//                  moving_resize_filter (moving_vox, moving_resized_vox);
//                  moving_smooth_filter (moving_resized_vox, moving_resized_smoothed_vox);
//                  template_resize_filter (template_vox, template_resized_vox);
//                  template_smooth_filter (template_resized_vox, template_resized_smoothed_vox);
//                }
//                metric.set_moving_image (moving_resized_smoothed_vox);
//                metric.set_template_image (template_resized_smoothed_vox);
                
//                ParamType parameters (transform, moving_resized_smoothed_vox, template_resized_smoothed_vox, midway_vox);

//                INFO ("neighbourhood kernel extent: " +str(kernel_extent));
//                parameters.set_extent(kernel_extent);

//                std::unique_ptr<MovingMaskVoxelType> moving_mask_vox;
//                std::unique_ptr<TemplateMaskVoxelType> template_mask_vox;
//                if (moving_mask) {
//                  moving_mask_vox.reset (new MovingMaskVoxelType (*moving_mask));
//                  parameters.moving_mask_interp.reset (new Image::Interp::Nearest<MovingMaskVoxelType> (*moving_mask_vox));
//                }
//                if (template_mask) {
//                  template_mask_vox.reset (new TemplateMaskVoxelType (*template_mask));
//                  parameters.template_mask_interp.reset (new Image::Interp::Nearest<TemplateMaskVoxelType> (*template_mask_vox));
//                }

//                Metric::Evaluate<MetricType, ParamType> evaluate (metric, parameters);
//                if (directions.is_set())
//                  evaluate.set_directions (directions);

//                Math::GradientDescent<Metric::Evaluate<MetricType, ParamType>,
//                                      typename TransformType::UpdateType > optim (evaluate, *transform.get_gradient_descent_updator());
//                // GradientDescent (Function& function, UpdateFunctor update_functor = LinearUpdate(), value_type step_size_upfactor = 3.0, value_type step_size_downfactor = 0.1)

//                optim.precondition (optimiser_weights);


//                optim.run (max_iter[level], grad_tolerance, false, step_tolerance, relative_cost_improvement_tolerance, gradient_criterion_tolerance, log_stream);
//                std::cerr << std::endl;
//                parameters.transformation.set_parameter_vector (optim.state());

//                if (log_stream) {
//                  std::ostream log_os (log_stream);
//                  log_os << "\n\n"; // two empty lines for gnuplot's index recognition
//                }
//                // Math::Vector<double> params = optim.state();
//                // VAR (optim.function_evaluations());
//                // Math::check_function_gradient (evaluate, params, 0.0001, true, optimiser_weights);
//              }
//            }

//        protected:
//          std::vector<int> max_iter;
//          std::vector<float> scale_factor;
//          std::vector<size_t> kernel_extent;
//          float grad_tolerance;
//          float step_tolerance;
//          float relative_cost_improvement_tolerance;
//          float gradient_criterion_tolerance;
//          std::streambuf* log_stream;
//          Transform::Init::InitType init_type;
//          Math::Matrix<float> directions;

//      };
//    }
//  }
//}

//#endif
