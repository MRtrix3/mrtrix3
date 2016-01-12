/*
 Copyright 2012 Brain Research Institute, Melbourne, Australia

 Written by David Raffelt, 24/02/2012

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

#ifndef __registration_transform_global_search_h__
#define __registration_transform_global_search_h__

#include <vector>
#include <iostream>     // std::streambuf
#include <Eigen/Geometry>
#include <Eigen/Eigen>


#include "math/math.h"
#include "image.h"
#include "debug.h"
// #include "image/average_space.h"
// #include "filter/normalise.h"
#include "filter/resize.h"
#include "filter/reslice.h"
#include "adapter/reslice.h"
#include "algo/threaded_loop.h"
#include "algo/copy.h"
#include "interp/linear.h"
#include "interp/nearest.h"
#include "registration/metric/params.h"
#include "registration/metric/cross_correlation.h"
#include "registration/metric/evaluate.h"
#include "registration/transform/initialiser.h"
#include "math/gradient_descent.h"
#include "math/check_gradient.h"
#include "math/rng.h"


namespace MR
{
  namespace Registration
  {

    class GlobalSearch
    {

      public:

        GlobalSearch () :
          max_iter (500),
          scale_factor (0.5),
          pool_size(7),
          loop_density (1.0),
          smooth_factor (1.0),
          grad_tolerance(1.0e-6),
          step_tolerance(1.0e-10),
          log_stream(nullptr),
          init_type (Transform::Init::mass) { assert(pool_size > 2); }

        void set_init_type (Transform::Init::InitType type) {
          init_type = type;
        }

        void set_log_stream (std::streambuf* stream) {
          log_stream = stream;
        }

        template <class MetricType, class TransformType, class Im1ImageType, class Im2ImageType, class Im1MaskType, class Im2MaskType>
        void run_masked (
          MetricType& metric,
          TransformType& transform,
          Im1ImageType& im1_image,
          Im2ImageType& im2_image,
          Im1MaskType& im1_mask,
          Im2MaskType& im2_mask) {

            INFO("running global search");

            typedef typename TransformType::ParameterType ParamType;
            // typedef Eigen::Transform<default_type, 3, Eigen::AffineProjective> TrafoType;
            typedef transform_type TrafoType;
            typedef Eigen::Matrix<default_type, 3, 3> MatType;
            typedef Eigen::Matrix<default_type, 3, 1> VecType;
            typedef Eigen::Quaternion<default_type> QuatType;

            typedef Image<float> MidwayImageType;
            typedef Im1ImageType ProcessedImageType;

            // smooth images
            Filter::Smooth im1_smooth_filter (im1_image);
            im1_smooth_filter.set_stdev(smooth_factor * 1.0 / (2.0 * scale_factor));
            auto im1_smoothed = Image<typename Im1ImageType::value_type>::scratch (im1_smooth_filter);

            Filter::Smooth im2_smooth_filter (im2_image);
            im2_smooth_filter.set_stdev(smooth_factor * 1.0 / (2.0 * scale_factor)) ;
            auto im2_smoothed = Image<typename Im2ImageType::value_type>::scratch (im2_smooth_filter);
            {
              LogLevelLatch log_level (0);
              im1_smooth_filter (im1_image, im1_smoothed);
              im2_smooth_filter (im2_image, im2_smoothed);
            }

            // --------------------------------- evaluate --------------------------

            typedef Interp::SplineInterp<Im1ImageType, Math::UniformBSpline<typename Im1ImageType::value_type>, Math::SplineProcessingType::ValueAndGradient> Im1ImageInterpolatorType;
            typedef Interp::SplineInterp<Im2ImageType, Math::UniformBSpline<typename Im2ImageType::value_type>, Math::SplineProcessingType::ValueAndGradient> Im2ImageInterpolatorType;
            typedef Interp::SplineInterp<ProcessedImageType, Math::UniformBSpline<typename ProcessedImageType::value_type>, Math::SplineProcessingType::ValueAndGradient> ProcessedImageInterpolatorType;
            typedef Image<bool> ProcessedMaskType;
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
                                   Interp::Nearest<ProcessedMaskType>> ParameterClassType;

            // create resized midway image
            std::vector<Eigen::Transform<double, 3, Eigen::Projective> > init_transforms;
            {
              Eigen::Transform<double, 3, Eigen::Projective> init_trafo_2 = transform.get_transform_half();
              Eigen::Transform<double, 3, Eigen::Projective> init_trafo_1 = transform.get_transform_half_inverse();
              init_transforms.push_back (init_trafo_2);
              init_transforms.push_back (init_trafo_1);
            }
            auto padding = Eigen::Matrix<default_type, 4, 1>(0.0, 0.0, 0.0, 0.0);
            default_type im2_res = 1.0;
            std::vector<Header> headers;
            headers.push_back(im2_image.original_header());
            headers.push_back(im1_image.original_header());
            auto midway_image_header = compute_minimum_average_header<default_type, Eigen::Transform<default_type, 3, Eigen::Projective>> (headers, im2_res, padding, init_transforms);
            auto midway_image = Header::scratch (midway_image_header).get_image<typename Im1ImageType::value_type>();

            // TODO: there must be a better way to do this without creating an empty image
            Filter::Resize midway_resize_filter (midway_image);
            midway_resize_filter.set_scale_factor (scale_factor);
            midway_resize_filter.set_interp_type (1);
            auto midway_resized = Image<typename Im1ImageType::value_type>::scratch (midway_resize_filter);
            {
              LogLevelLatch log_level (0);
              midway_resize_filter (midway_image, midway_resized);
            }

            ParameterClassType parameters (transform, im1_smoothed, im2_smoothed, midway_resized, im1_mask, im2_mask);
            parameters.loop_density = loop_density;
            // set control point coordinates inside +-1/3 of the midway_image size
            // {
            //   Eigen::Vector3 ext (midway_image_header.spacing(0) / 6.0,
            //                       midway_image_header.spacing(1) / 6.0,
            //                       midway_image_header.spacing(2) / 6.0);
            //   for (size_t i = 0; i<3; ++i)
            //     ext(i) *= midway_image_header.size(i) - 0.5;
            //   parameters.set_control_points_extent(ext);
            // }

            // Metric::Evaluate<MetricType, ParameterClassType> evaluate (metric, parameters);

            Eigen::Matrix<default_type, Eigen::Dynamic, 1> x;
            Eigen::Matrix<default_type, Eigen::Dynamic, 1> gradient;

            // --------------------------------- evaluate --------------------------

            // auto out = Image<float>::create ("/tmp/im1.mif", im1_resized);
            // copy(im1_resized, out);

/*            template<typename Scalar, int Dim, int Mode, int Options>
            template<typename RotationMatrixType, typename ScalingMatrixType>
            void Transform<Scalar,Dim,Mode,Options>::computeRotationScaling(RotationMatrixType *rotation, ScalingMatrixType *scaling) const
            {
              JacobiSVD<LinearMatrixType> svd(linear(), ComputeFullU | ComputeFullV);

              Scalar x = (svd.matrixU() * svd.matrixV().adjoint()).determinant(); // so x has absolute value 1
              VectorType sv(svd.singularValues());
              sv.coeffRef(0) *= x;
              if(scaling) scaling->lazyAssign(svd.matrixV() * sv.asDiagonal() * svd.matrixV().adjoint());
              if(rotation)
              {
                LinearMatrixType m(svd.matrixU());
                m.col(0) /= x;
                rotation->lazyAssign(m * svd.matrixV().adjoint());
              }
            }*/
            // Eigen::Scaling(sx, sy, sz)
            // Eigen::Translation<default_type,3>(tx, ty, tz)
            // Eigen::Quaternion<default_type> q;
            // q = AngleAxis<default_type>(angle_in_radian, axis);

            // Eigen::Quaternion<float> q(R);
            // vector<tuple<int, string>> v;

            // MatType M_start (transform.get_matrix());
            QuatType q_start(transform.get_matrix());
            VecType t_start = transform.get_translation();
            // nonorthonormal_matrix2quaternion(M_start, Q_start);

            // std::cerr << q_start << std::endl;

            std::vector<std::tuple<default_type, QuatType, VecType>> parental;
            std::vector<std::tuple<default_type, QuatType, VecType>> filial;
            parental.push_back(std::make_tuple(std::numeric_limits<default_type>::max(), q_start, t_start));
            for (size_t i = 1; i < pool_size; ++i){
              QuatType q (q_start);
              mutate(q);
              VecType t (t_start);
              mutate(t);
              parental.push_back(std::make_tuple(std::numeric_limits<default_type>::max(), q, t));
            }

            std::sort(std::begin(parental), std::end(parental), TupleCompare<0>());

            INFO("global search done.");
          }

      protected:
        size_t max_iter;
        default_type scale_factor;
        size_t pool_size;
        default_type loop_density;
        default_type smooth_factor;
        std::vector<size_t> kernel_extent;
        default_type grad_tolerance;
        default_type step_tolerance;
        std::streambuf* log_stream;
        Transform::Init::InitType init_type;
        size_t gd_repetitions;

      private:
        template <class TransformType>
        void crossover(const TransformType& P1, const TransformType& P2, TransformType& F) {
          // TODO
        }

        template <class ComputeType>
        void mutate(Eigen::Quaternion<ComputeType>& q, ComputeType angle = 0.2) {
          Eigen::Matrix<ComputeType,3,1> axis = Eigen::Matrix<ComputeType,3,1>::Random();
          axis.array() *= (1.0 / axis.norm());
          Eigen::AngleAxis<ComputeType> aa (angle, axis );
          q = q * aa;
        }

        template <class ComputeType>
        void mutate(Eigen::Matrix<ComputeType, 3, 1>& T, ComputeType distance = 50) {
          Eigen::Matrix<ComputeType,3,1> direction = Eigen::Matrix<ComputeType,3,1>::Random();
          direction.array() *= (1.0 / direction.norm());
          T += distance * direction;
        }

        template <class MatrixType, class QuaternionType = Eigen::Quaternion<default_type>>
        void nonorthonormal_matrix2quaternion (const MatrixType& matrix, QuaternionType& q) {
          // http://people.csail.mit.edu/bkph/articles/Nearest_Orthonormal_Matrix.pdf
          Eigen::JacobiSVD<MatrixType> svd(matrix, Eigen::ComputeFullV);
          MatrixType M = svd.matrixV().col(0) * svd.matrixV().col(0).transpose() / svd.singularValues()(0) +
                         svd.matrixV().col(1) * svd.matrixV().col(1).transpose() / svd.singularValues()(1) +
                         svd.matrixV().col(2) * svd.matrixV().col(2).transpose() / svd.singularValues()(2);
          q = QuaternionType(MatrixType(matrix * M));
        }

        template<size_t M, template<typename> class F = std::less>
        struct TupleCompare {
          template<typename T>
          bool operator()(T const &t1, T const &t2) {
            return F<typename std::tuple_element<M, T>::type>()(std::get<M>(t1), std::get<M>(t2));
          }
        };
    };
  }
}

#endif
