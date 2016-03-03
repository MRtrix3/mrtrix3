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

#ifndef __registration_transform_global_search_h__
#define __registration_transform_global_search_h__

#include <vector>
#include <iostream>
#include <Eigen/Geometry>
#include <Eigen/Eigen>

#include "math/math.h"
#include "math/median.h"
#include "math/rng.h"
#include "math/gradient_descent.h"
#include "image.h"
#include "debug.h"
#include "image/average_space.h"
#include "filter/resize.h"
#include "filter/reslice.h"
#include "adapter/reslice.h"
#include "interp/linear.h"
#include "interp/cubic.h"
#include "interp/nearest.h"
#include "registration/metric/mean_squared.h"
#include "registration/metric/mean_squared_no_gradient.h"
#include "registration/metric/params.h"
#include "registration/metric/cross_correlation.h"
#include "registration/metric/evaluate.h"
#include "registration/metric/thread_kernel.h"
#include "registration/transform/initialiser.h"
#include "registration/transform/rigid.h"
#include "progressbar.h"
#include "file/config.h"

namespace MR
{
  namespace Registration
  {
    namespace GlobalSearch
    {

      typedef transform_type TrafoType;
      typedef Eigen::Matrix<default_type, 3, 3> MatType;
      typedef Eigen::Matrix<default_type, 3, 1> VecType;
      typedef Eigen::Quaternion<default_type> QuatType;

      template <class MetricType = Registration::Metric::MeanSquaredNoGradient>
        class ExhaustiveRotationSearch {
          public:
            ExhaustiveRotationSearch (
              Image<default_type>& image1,
              Image<default_type>& image2,
              Image<default_type>& mask1,
              Image<default_type>& mask2,
              MetricType& metric) :
            im1(image1),
            im2(image2),
            mask1(mask1),
            mask2(mask2),
            metric (metric),
            global_search_iterations (10000),
            rot_angles (9),
            local_search_directions (250),
            idx_angle (0),
            idx_dir (0) {
              rot_angles[0] = 2.0;
              rot_angles[1] = 5.0;
              rot_angles[2] = 10.0;
              rot_angles[3] = 15.0;
              rot_angles[4] = 20.0;
              rot_angles[5] = 25.0;
              rot_angles[6] = 30.0;
              rot_angles[7] = 35.0;
              rot_angles[8] = 40.0;
            };

            typedef Metric::Params<Registration::Transform::Rigid,
                                     Image<default_type>,
                                     Image<default_type>,
                                     Image<default_type>,
                                     Image<default_type>,
                                     Image<default_type>,
                                     // use Interp::LinearInterpProcessingType::ValueAndDerivative for metric that calculates gradients
                                     Interp::LinearInterp<Image<default_type>, Interp::LinearInterpProcessingType::Value>,
                                     Interp::LinearInterp<Image<default_type>, Interp::LinearInterpProcessingType::Value>,
                                     Interp::Linear<Image<default_type>>,
                                     Interp::Linear<Image<default_type>>,
                                     Image<default_type>,
                                     Interp::LinearInterp<Image<default_type>, Interp::LinearInterpProcessingType::Value>,
                                     Image<default_type>,
                                     Interp::Nearest<Image<default_type>>> ParamType;

            void write_images (const std::string& im1_path, const std::string& im2_path) {
              Image<default_type> image1_midway;
              Image<default_type> image2_midway;

              Header image1_midway_header (midway_image_header);
              image1_midway_header.datatype() = DataType::Float64;
              image1_midway_header.set_ndim(im1.ndim());
              for (size_t dim = 3; dim < im1.ndim(); ++dim){
                image1_midway_header.spacing(dim) = im1.spacing(dim);
                image1_midway_header.size(dim) = im1.size(dim);
              }
              image1_midway = Image<default_type>::create (im1_path, image1_midway_header);
              Header image2_midway_header (midway_image_header);
              image2_midway_header.datatype() = DataType::Float64;
              image2_midway_header.set_ndim(im2.ndim());
              for (size_t dim = 3; dim < im2.ndim(); ++dim){
                image2_midway_header.spacing(dim) = im2.spacing(dim);
                image2_midway_header.size(dim) = im2.size(dim);
              }
              image2_midway = Image<default_type>::create (im2_path, image2_midway_header);

              Filter::reslice<Interp::Cubic> (im1, image1_midway, transform.get_transform_half(), Adapter::AutoOverSample, 0.0);
              Filter::reslice<Interp::Cubic> (im2, image2_midway, transform.get_transform_half_inverse(), Adapter::AutoOverSample, 0.0);
            }

            transform_type get_best_trafo() const {
              transform_type best = best_trafo;
              return best;
            }

            Eigen::Vector3d get_centre() const {
              Eigen::Vector3d c = centre;
              return c;
            }

            void set_rot_angles (const std::vector<default_type> angles) {
              assert (angles.size() > 0);
              for (size_t i=0; i<angles.size(); ++i){
                if (angles[i] < 0.0 or angles[i] > Math::pi)
                  throw Exception ("rotation search angle has to be in the range [0...pi]");
              }
              rot_angles = angles;
            }

            Eigen::Vector3d get_offset() const {
              Eigen::Vector3d o = offset;
              return o;
            }

            void run (
              default_type image_scale_factor = 0.1,
              bool global_search = false,
              bool debug = false) {

              std::string what = global_search? "global" : "local";
              size_t iterations = global_search? global_search_iterations : (rot_angles.size() * local_search_directions);
              ProgressBar progress ("performing " + what + " search for best rotation", iterations);
              overlap_it.resize (iterations);
              cost_it.resize (iterations);
              trafo_it.reserve (iterations);

              if (!global_search) {
                gen_uniform_rotation_axes (local_search_directions, 180.0); // full sphere
                az_el_to_cartesian();
              }
              ParamType parameters = get_parameters (image_scale_factor);
              Eigen::Matrix<default_type, Eigen::Dynamic, 1> gradient (parameters.transformation.size());
              size_t iteration (0);
              ssize_t cnt (0);
              default_type cost (0);
              Metric::ThreadKernel<MetricType, ParamType> kernel (metric, parameters, cost, gradient, &cnt);
              ThreadedLoop (parameters.midway_image, 0, 3).run (kernel);
              assert (cnt > 0);
              overlap_it[0] = cnt;
              cost_it[0] = cost / static_cast<default_type>(cnt);
              transform_type T = parameters.transformation.get_transform();
              trafo_it.push_back (T);

              // parameters.transformation.debug();
              offset = parameters.transformation.get_translation();
              centre = parameters.transformation.get_centre();
              transform_type Tc2, To, R0;
              Tc2.setIdentity();
              To.setIdentity();
              R0.setIdentity();
              To.translation() = offset;
              Tc2.translation() = centre - 0.5 * offset;
              while ( ++iteration < iterations ) {
                ++progress;
                if (global_search) {
                  gen_random_quaternion ();
                }
                else {
                  gen_local_quaternion ();
                }
                R0.linear() = quat.matrix();
                T = Tc2 * To * R0 * Tc2.inverse();
                parameters.transformation.set_transform (T);
                cost = 0.0;
                cnt = 0;
                Metric::ThreadKernel<MetricType, ParamType> kernel (metric, parameters, cost, gradient, &cnt);
                ThreadedLoop (parameters.midway_image, 0, 3).run (kernel);
                DEBUG ("rotation search: iteration " + str(iteration) + " cost: " + str(cost) + " cnt: " + str(cnt));
                // write_images ( "im1_" + str(iteration) + ".mif", "im2_" + str(iteration) + ".mif");
                overlap_it[iteration] = cnt;
                cost_it[iteration] = cost / static_cast<default_type>(cnt);
                trafo_it.push_back (T);
              }
              if (debug) {
                save_matrix(cost_it, "/tmp/cost_before.txt");
                save_matrix(overlap_it, "/tmp/overlap.txt");
              }
              //  best trafo := lowest cost per voxel with at least mean overlap
              {
                auto max_ = Eigen::MatrixXd::Constant(cost_it.rows(), 1, std::numeric_limits<default_type>::max());
                default_type mean_overlap = static_cast<default_type>(overlap_it.sum()) / static_cast<default_type>(iterations);
                // reject solutions with less than mean overlap by setting cost to max
                cost_it = (overlap_it.array() > mean_overlap).select(cost_it, max_);
                std::ptrdiff_t i;
                min_cost = cost_it.minCoeff(&i);
                T = trafo_it[i];
                best_trafo = T;
              }
              if (debug) {
                save_matrix(cost_it, "/tmp/cost_after.txt");
                Eigen::VectorXd t(2);
                t(0) = cost_it(0);
                t(1) = min_cost;
                save_matrix(t, "/tmp/cost_mass_chosen.txt");
                save_matrix(centre, "/tmp/centre.txt");
                parameters.transformation.set_transform (best_trafo);
                write_images ( "/tmp/im1_best.mif", "/tmp/im2_best.mif");
              }
            };

          private:
            ParamType get_parameters (default_type& image_scale_factor) {
              {
                LogLevelLatch log_level (0);
                Registration::Transform::Init::initialise_using_image_mass (im1, im2, mask1, mask2, transform);
              }

              // create resized midway image
              std::vector<Eigen::Transform<default_type, 3, Eigen::Projective> > init_transforms;
              {
                Eigen::Transform<default_type, 3, Eigen::Projective> init_trafo_1 = transform.get_transform_half_inverse();
                Eigen::Transform<default_type, 3, Eigen::Projective> init_trafo_2 = transform.get_transform_half();
                init_transforms.push_back (init_trafo_1);
                init_transforms.push_back (init_trafo_2);
              }
              auto padding = Eigen::Matrix<default_type, 4, 1>(0.0, 0.0, 0.0, 0.0);
              default_type im_res = 1.0;
              std::vector<Header> headers;
              headers.push_back(im1.original_header());
              headers.push_back(im2.original_header());
              midway_image_header = compute_minimum_average_header<default_type, Eigen::Transform<default_type, 3, Eigen::Projective>> (headers, im_res, padding, init_transforms);
              midway_image = Header::scratch (midway_image_header).template get_image<default_type>();

              // TODO: there must be a way to do this without creating a scratch image...
              Filter::Resize midway_resize_filter (midway_image);
              midway_resize_filter.set_scale_factor (image_scale_factor);
              midway_resize_filter.set_interp_type (1);
              midway_resized = Image<default_type>::scratch (midway_resize_filter);
              {
                LogLevelLatch log_level (0);
                midway_resize_filter (midway_image, midway_resized);
              }

              ParamType parameters (transform, im1, im2, midway_resized, mask1, mask2);
              parameters.loop_density = 1.0;
              return parameters;
            }

            // gen_random_quaternion generates random quaternion (rotation around random direction
            // by random angle)
            inline void gen_random_quaternion () {
              Eigen::Matrix<default_type, 4, 1> v(rndn(), rndn(), rndn(), rndn());
              v.array() /= v.norm();
              quat = Eigen::Quaternion<default_type> (v);
            };

            // gen_uniform_rotation_axes generates roughly uniformly distributed points on sphere
            // starting on z-axis up to -z-axis (max_cone_angle_deg=180). points are stored as matrix
            // of azimuth and elevation
            void gen_uniform_rotation_axes ( const size_t& n_dir, const default_type& max_cone_angle_deg ) {
              assert (n_dir > 1);
              assert (max_cone_angle_deg > 0.0);
              assert (max_cone_angle_deg <= 180.0);

              const default_type golden_ratio ((1.0 + std::sqrt (5.0)) / 2.0);
              const default_type golden_angle (2.0 * Math::pi * (1.0 - 1.0 / golden_ratio));

              az_el.resize (n_dir,2);
              Eigen::Matrix<default_type, Eigen::Dynamic, 1> idx (n_dir);
              for (size_t i = 0; i < n_dir; ++i)
                idx(i) = i;
              az_el.col(0) = idx * golden_angle;

              // el(i) = acos (1-(1-cosd(max_cone_angle_deg))*i/(n_dir-1) )
              default_type a = (1.0 - std::cos(Math::pi / 180.0 * default_type (max_cone_angle_deg))) / (default_type (n_dir - 1));
              az_el.col(1).array() = - a * idx.array() + 1.0;
              for (size_t i = 0; i < n_dir; ++i)
                az_el(i, 1) = std::acos (az_el(i, 1));
            }

            // convert spherical coordinates (az_el) to cartesian coordinates (xyz)
            inline void az_el_to_cartesian () {
              xyz.resize (az_el.rows(), 3);
              Eigen::VectorXd el_sin = az_el.col(1).array().sin();
              xyz.col(0).array() = el_sin.array() * az_el.col(0).array().cos();
              xyz.col(1).array() = el_sin.array() * az_el.col(0).array().sin();
              xyz.col(2).array() = az_el.col(1).array().cos();
            }

            inline void gen_local_quaternion () {
              // Eigen::Vector3d normal ( -std::cos (az_el(idx,1)), -std::sin (az_el(idx,0)) * std::cos (az_el(idx,0)), 0.0);
              // Eigen::AngleAxis<default_type> aa (az_el(idx,1), normal);
              // Eigen::Vector3d normal ( xyz.row(idx) );
              // Eigen::AngleAxis<default_type> aa (angle, normal);
              if (idx_dir == local_search_directions) {
                idx_dir = 0;
                ++idx_angle;
                assert (idx_angle < rot_angles.size());
              }
              quat = Eigen::Quaternion<default_type> ( Eigen::AngleAxis<default_type> (rot_angles[idx_angle], xyz.row(idx_dir)) );
              ++idx_dir;
            }

            Image<default_type> im1, im2, mask1, mask2, midway_image, midway_resized;
            MetricType metric;
            Math::RNG::Normal<default_type> rndn;
            Eigen::Quaternion<default_type> quat;
            transform_type best_trafo;
            Header midway_image_header;
            default_type min_cost;
            std::vector<default_type> vec_cost;
            std::vector<size_t> vec_overlap;
            size_t global_search_iterations;
            std::vector<default_type> rot_angles;
            size_t local_search_directions;
            size_t idx_angle, idx_dir;
            Eigen::Vector3d centre, offset;
            Registration::Transform::Rigid transform;
            Eigen::Matrix<default_type, Eigen::Dynamic, 2> az_el;
            Eigen::Matrix<default_type, Eigen::Dynamic, 3> xyz;
            Eigen::Matrix<default_type, Eigen::Dynamic, 1> overlap_it, cost_it;
            std::vector<transform_type> trafo_it;
          };

      template <class MatrixType, class QuaternionType = Eigen::Quaternion<default_type>>
        void nonorthonormal_matrix2quaternion (const MatrixType& matrix, QuaternionType& q) {
          // http://people.csail.mit.edu/bkph/articles/Nearest_Orthonormal_Matrix.pdf
          Eigen::JacobiSVD<MatrixType> svd(matrix, Eigen::ComputeFullV);
          MatrixType M = svd.matrixV().col(0) * svd.matrixV().col(0).transpose() / svd.singularValues()(0) +
                         svd.matrixV().col(1) * svd.matrixV().col(1).transpose() / svd.singularValues()(1) +
                         svd.matrixV().col(2) * svd.matrixV().col(2).transpose() / svd.singularValues()(2);
          q = QuaternionType(MatrixType(matrix * M));
        }

      // template<typename RotationMatrixType, typename ScalingMatrixType>
      //   void Transform<Scalar,Dim,Mode,Options>::computeRotationScaling(RotationMatrixType *rotation, ScalingMatrixType *scaling) const
      //   {
      //     JacobiSVD<LinearMatrixType> svd(linear(), ComputeFullU | ComputeFullV);

      //     Scalar x = (svd.matrixU() * svd.matrixV().adjoint()).determinant(); // so x has absolute value 1
      //     VectorType sv(svd.singularValues());
      //     sv.coeffRef(0) *= x;
      //     if(scaling) scaling->lazyAssign(svd.matrixV() * sv.asDiagonal() * svd.matrixV().adjoint());
      //     if(rotation)
      //     {
      //       LinearMatrixType m(svd.matrixU());
      //       m.col(0) /= x;
      //       rotation->lazyAssign(m * svd.matrixV().adjoint());
      //     }

      template <class ComputeType = default_type>
        class Candidate {
          public:
            Candidate (const TrafoType& T, const default_type& scale, const default_type& density, const size_t& max_GD_iter):
              trafo (T),
              q (T.rotation()),
              translation (T.translation()),
              cost (std::numeric_limits<ComputeType>::max()),
              overlap (-1),
              scale_f (scale),
              loop_d (density),
              max_GD_iter (max_GD_iter) {
                MatType M(trafo.linear());
                nonorthonormal_matrix2quaternion(M, q);
              }

            Candidate (const QuatType& quat, const VecType& translation, const default_type& scale, const default_type& density, const size_t& max_GD_iter):
              trafo (quat.matrix()),
              q (quat),
              translation (translation),
              cost (std::numeric_limits<ComputeType>::max()),
              overlap (-1),
              scale_f (scale),
              loop_d (density),
              max_GD_iter (max_GD_iter) {
              trafo.linear() = quat.matrix();
              trafo.translation() = translation;
            }

            void crossover_affine (const default_type& t, const Candidate<ComputeType>& p, std::vector<Candidate<ComputeType>>& filial) {
              assert (t > 0);
              assert (t < 1.0);
              Eigen::MatrixXd M1 = trafo.linear();
              Eigen::MatrixXd M2 = p.get_trafo().linear();
              assert (M1.determinant() * M2.determinant() > 0);

              Eigen::Matrix3d R1 = trafo.rotation();
              Eigen::Matrix3d R2 = p.get_trafo().rotation();
              Eigen::Quaterniond Q1(R1);
              Eigen::Quaterniond Q2(R2);
              Eigen::Quaterniond Qout;

              // get stretch (shear becomes rotation and stretch)
              Eigen::MatrixXd S1 = R1.transpose() * M1;
              Eigen::MatrixXd S2 = R2.transpose() * M2;
              assert (M1.isApprox(R1*S1));
              assert (M2.isApprox(R2*S2));

              TrafoType T1; //, T2;
              T1.translation() = ((1.0 - t) * trafo.translation() + t * p.get_trafo().translation());
              Qout = Q1.slerp(t, Q2);
              T1.linear() = Qout * ((1.0 - t) * S1 + t * S2);
              filial.push_back({T1, scale_f, loop_d, max_GD_iter});

              // T2.translation() = ( t * trafo.translation() + (1.0 - t) * p.get_trafo().translation());
              // Qout = Q1.slerp(1.0 - t, Q2);
              // T2.linear() = Qout * (t * S1 + (1.0 - t) * S2);
              // filial.push_back({T2, scale_f, loop_d, max_GD_iter});
            }

            void crossover_rigid (const default_type& t, const Candidate<ComputeType>& p, std::vector<Candidate<ComputeType>>& filial) {
              assert (t > 0);
              assert (t < 1.0);

              Eigen::Quaterniond Q2(p.get_quaternion());
              Eigen::Quaterniond Qout;

              VecType t1 = ((1.0 - t) * trafo.translation() + t * p.get_trafo().translation());
              Qout = q.slerp(t, Q2);
              filial.push_back({Qout, t1, scale_f, loop_d, max_GD_iter});

              // VecType t2 = ( t * trafo.translation() + (1.0 - t) * p.get_trafo().translation());
              // Qout = q.slerp(1.0 - t, Q2);
              // filial.push_back({Qout, t2, scale_f, loop_d, max_GD_iter});
            }

            void mutate_rigid (ComputeType angle = 0.2, ComputeType distance = 30) {
              mutate(q, angle);
              mutate(translation, distance);
              trafo.linear() = q.matrix();
              trafo.translation() = translation;
            }

            template <class MetricType, class TransformType, class Im1ImageType, class Im2ImageType, class Im1MaskType, class Im2MaskType, class MidwayImageType = Im1ImageType, class ProcessedImageType = Im1ImageType>
            Metric::Params<TransformType,
                                     Im1ImageType,
                                     Im2ImageType,
                                     MidwayImageType,
                                     Im1MaskType,
                                     Im2MaskType,
                                     Interp::LinearInterp<Im1ImageType, Interp::LinearInterpProcessingType::ValueAndDerivative>,
                                     Interp::LinearInterp<Im2ImageType, Interp::LinearInterpProcessingType::ValueAndDerivative>,
                                     Interp::Linear<Im1MaskType>,
                                     Interp::Linear<Im2MaskType>,
                                     ProcessedImageType,
                                     Interp::LinearInterp<ProcessedImageType, Interp::LinearInterpProcessingType::ValueAndDerivative>,
                                     Image<bool>,
                                     Interp::Nearest<Image<bool>>> init_parameters (MetricType& metric,
              TransformType& transform,
              Im1ImageType& im1_image,
              Im2ImageType& im2_image,
              Im1MaskType& im1_mask,
              Im2MaskType& im2_mask) {

              typedef Metric::Params<TransformType,
                                     Im1ImageType,
                                     Im2ImageType,
                                     MidwayImageType,
                                     Im1MaskType,
                                     Im2MaskType,
                                     Interp::LinearInterp<Im1ImageType, Interp::LinearInterpProcessingType::ValueAndDerivative>,
                                     Interp::LinearInterp<Im2ImageType, Interp::LinearInterpProcessingType::ValueAndDerivative>,
                                     Interp::Linear<Im1MaskType>,
                                     Interp::Linear<Im2MaskType>,
                                     ProcessedImageType,
                                     Interp::LinearInterp<ProcessedImageType, Interp::LinearInterpProcessingType::ValueAndDerivative>,
                                     Image<bool>,
                                     Interp::Nearest<Image<bool>>> ParamType;

              // create resized midway image
              std::vector<Eigen::Transform<double, 3, Eigen::Projective> > init_transforms;
              {
                Eigen::Transform<double, 3, Eigen::Projective> init_trafo_1 = transform.get_transform_half_inverse();
                Eigen::Transform<double, 3, Eigen::Projective> init_trafo_2 = transform.get_transform_half();
                init_transforms.push_back (init_trafo_1);
                init_transforms.push_back (init_trafo_2);
              }
              auto padding = Eigen::Matrix<default_type, 4, 1>(0.0, 0.0, 0.0, 0.0);
              default_type im_res = 1.0;
              std::vector<Header> headers;
              headers.push_back(im1_image.original_header());
              headers.push_back(im2_image.original_header());
              midway_image_header = compute_minimum_average_header<default_type, Eigen::Transform<default_type, 3, Eigen::Projective>> (headers, im_res, padding, init_transforms);
              auto midway_image = Header::scratch (midway_image_header).template get_image<typename Im1ImageType::value_type>();

              // TODO: there must be a way to do this without creating a scratch image...
              Filter::Resize midway_resize_filter (midway_image);
              midway_resize_filter.set_scale_factor (scale_f);
              midway_resize_filter.set_interp_type (1);
              auto midway_resized = Image<typename Im1ImageType::value_type>::scratch (midway_resize_filter);
              {
                LogLevelLatch log_level (0);
                midway_resize_filter (midway_image, midway_resized);
              }

              // set control point coordinates inside +-1/3 of the midway_image size
              ParamType parameters (transform, im1_image, im2_image, midway_resized, im1_mask, im2_mask);
              {
                Eigen::Vector3 ext (midway_image_header.spacing(0) / 6.0,
                                    midway_image_header.spacing(1) / 6.0,
                                    midway_image_header.spacing(2) / 6.0);
                for (size_t i = 0; i<3; ++i)
                  ext(i) *= midway_image_header.size(i) - 0.5;
                parameters.set_control_points_extent(ext);
              }
              return parameters;
            }

            template <class MetricType, class TransformType, class Im1ImageType, class Im2ImageType, class Im1MaskType, class Im2MaskType>
            void run_sgd (MetricType& metric,
              TransformType& transform,
              Im1ImageType& im1_image,
              Im2ImageType& im2_image,
              Im1MaskType& im1_mask,
              Im2MaskType& im2_mask) {

              transform.set_transform(trafo);
              auto parameters = init_parameters(metric, transform, im1_image, im2_image, im1_mask, im2_mask);
              typedef decltype (parameters) ParamType;
              parameters.loop_density = loop_d;

              // SGD
              Metric::Evaluate<MetricType, ParamType> evaluate (metric, parameters);
              Math::GradientDescent<Metric::Evaluate<MetricType, ParamType>, typename TransformType::UpdateType>
              optim (evaluate, *transform.get_gradient_descent_updator());
              Eigen::Matrix<typename TransformType::ParameterType, Eigen::Dynamic, 1> optimiser_weights = transform.get_optimiser_weights();
              optim.precondition (optimiser_weights);
              optim.run (max_GD_iter, 1.0e-30, log_stream);
              parameters.transformation.set_parameter_vector (optim.state());

              trafo = parameters.transformation.get_transform();
              MatType M(trafo.linear());
              nonorthonormal_matrix2quaternion(M, q);
              translation = trafo.translation();

              // get cost
              parameters.loop_density = 1.0;
              Eigen::Matrix<default_type, Eigen::Dynamic, 1> gradient;
              gradient.resize(transform.size());
              ComputeType c(0.0);
              ssize_t cnt(0);
              Metric::ThreadKernel<MetricType, ParamType> kernel (metric, parameters, c, gradient, &cnt);
              ThreadedLoop (parameters.midway_image, 0, 3).run (kernel);
              cost = c;
              overlap = cnt;
            }

            template <class MetricType, class TransformType, class Im1ImageType, class Im2ImageType, class Im1MaskType, class Im2MaskType>
            void evaluate (MetricType& metric,
              TransformType& transform,
              Im1ImageType& im1_image,
              Im2ImageType& im2_image,
              Im1MaskType& im1_mask,
              Im2MaskType& im2_mask) {

              transform.set_transform(trafo);
              auto parameters = init_parameters(metric, transform, im1_image, im2_image, im1_mask, im2_mask);
              typedef decltype (parameters) ParamType;
              parameters.loop_density = 1.0;

              Eigen::Matrix<default_type, Eigen::Dynamic, 1> gradient;
              gradient.resize(transform.size());
              ComputeType c;
              ssize_t cnt(0);
              Metric::ThreadKernel<MetricType, ParamType> kernel (metric, parameters, c, gradient, &cnt);
              ThreadedLoop (parameters.midway_image, 0, 3).run (kernel);
              cost = c;
              overlap = cnt;
            }

          ComputeType get_cost () const {
            return cost;
          }

          ComputeType get_overlap () const {
            return overlap;
          }

          ComputeType get_weighted_cost () const {
            return cost / std::pow((ComputeType) overlap, 1.2);
          }

          TrafoType get_trafo () const {
            return trafo;
          }

          QuatType get_quaternion () const {
            return q;
          }

          VecType get_translation () const {
            return translation;
          }

          bool operator<(const Candidate& a) const
          {
            if (overlap <= 0)
              return false;
            if (a.get_overlap() <= 0)
              return true;
            return get_weighted_cost() < a.get_weighted_cost();
          }

          operator std::string () const
          {
            return str(cost, 10) + " " + str(overlap) + " " +
            str(trafo.matrix().row(0), 5) + " " +
            str(trafo.matrix().row(1), 5) + " " +
            str(trafo.matrix().row(2), 5);
          }

          friend std::ostream& operator<< (std::ostream &out,
                                   const Candidate<ComputeType> &c) {
            out << std::string(c);
            return out;
          }

          Header midway_image_header;

          private:
              void mutate (Eigen::Quaternion<ComputeType>& q, ComputeType angle = 0.2) {
                Eigen::Matrix<ComputeType,3,1> axis = Eigen::Matrix<ComputeType,3,1>::Random();
                axis.array() *= (1.0 / axis.norm());
                Eigen::AngleAxis<ComputeType> aa (angle, axis );
                q = q * aa;
              }

              void mutate (Eigen::Matrix<ComputeType, 3, 1>& T, ComputeType distance = 30) {
                Eigen::Matrix<ComputeType,3,1> direction = Eigen::Matrix<ComputeType,3,1>::Random();
                direction.array() *= (1.0 / direction.norm());
                T += distance * direction;
              }

          protected:
            TrafoType trafo;
            QuatType q;
            VecType translation;
            ComputeType cost;
            ssize_t overlap;
            ComputeType scale_f;
            ComputeType loop_d;
            size_t max_GD_iter;
            std::streambuf* log_stream;
        }; // Candidate

      template < class ComputeType>
      bool lower_cost (const Candidate<ComputeType>& a, const Candidate<ComputeType>& b) {
       return a.get_cost() < b.get_cost();
      }

      class GlobalSearch
      {
        public:
          GlobalSearch () :
            max_iter (7),
            max_GD_iter (15),
            scale_factor (0.5),
            pool_size (7),
            mutation_rad (File::Config::get_float ("reg_mutation_rad", 0.2)),
            mutation_t_wrt_fov (File::Config::get_float ("reg_mutation_t_wrt_fov", 0.1)),
            loop_density (1.0),
            smooth_factor (1.0),
            log_stream (nullptr) {  }

          void set_iterations (size_t& iterations) {
            max_iter = iterations;
          }

          void set_gradient_descent_iterations (size_t& iterations) {
            max_GD_iter = iterations;
          }

          void set_pool_size (size_t& size) {
            if (size < 2) throw Exception ("global search pool size has to be larger than 2");
            pool_size = size;
          }

          void set_mutation_rotation (default_type& radians) {
            if ((radians < 0.0) or (radians > Math::pi_2)) throw Exception (
              "rotation in radians has to be smaller than 2*pi and non-negative");
            mutation_rad = radians;
          }

          void set_mutation_translation_wrt_fov (default_type& percent) {
            if ((percent < 0.0) or (percent > 100.0)) throw Exception ("");
            mutation_t_wrt_fov = percent;
          }

          void set_log_stream (std::streambuf* stream) {
            log_stream = stream;
          }

          void set_gradient_descent_loop_density (default_type density) {
            if ((density <= default_type(0.0)) or (density > default_type(1.0)))
              throw Exception ("gradient descent loop density has to be in the interval (0, 1]");
            loop_density = density;
          }

          template <class MetricType, class TransformType, class Im1ImageType, class Im2ImageType, class Im1MaskType, class Im2MaskType>
          void run_masked (
            MetricType& metric,
            TransformType& transform,
            Im1ImageType& im1_image,
            Im2ImageType& im2_image,
            Im1MaskType& im1_mask,
            Im2MaskType& im2_mask) {

              ProgressBar progress ("performing global search", pool_size * (max_iter+1));
              std::ostream log_os(log_stream? log_stream : std::cerr.rdbuf());

              // smooth images,
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

              TrafoType T_init (transform.get_matrix());
              T_init.translation() = transform.get_translation();

              std::vector<Candidate<default_type>> parental, geneology;
              parental.push_back ({T_init, scale_factor, loop_density, max_GD_iter});
              parental.back().run_sgd (metric, transform, im1_smoothed, im2_smoothed, im1_mask, im2_mask);

              default_type spatial_extent(0.0);
              {
                std::vector<default_type> spatial_ext_v;
                spatial_ext_v.push_back(parental.back().midway_image_header.spacing(0) * (parental.back().midway_image_header.size(0) - 0.5));
                spatial_ext_v.push_back(parental.back().midway_image_header.spacing(1) * (parental.back().midway_image_header.size(1) - 0.5));
                spatial_ext_v.push_back(parental.back().midway_image_header.spacing(2) * (parental.back().midway_image_header.size(2) - 0.5));
                spatial_extent += spatial_ext_v[0];
                spatial_extent += spatial_ext_v[1];
                spatial_extent += spatial_ext_v[2];
                spatial_extent /= 3.0; // TODO use vector instead of mean
              }

              while (parental.size() < pool_size){
                parental.push_back ({T_init, scale_factor, loop_density, max_GD_iter});
                parental.back().mutate_rigid(mutation_rad, mutation_t_wrt_fov*spatial_extent);
                parental.back().run_sgd (metric, transform, im1_smoothed, im2_smoothed, im1_mask, im2_mask);
                if (parental.back().get_overlap() == 0 or parental.back().get_cost() == 0.0)
                  parental.pop_back();
                else {
                  ++progress;
                  if (log_stream) log_os << str(parental.back()) << std::endl;
                }
              }
              if (log_stream) log_os.flush();

              for(auto const& p: parental) {
                geneology.push_back(p);
              }

              // go to next generation: cross parental generation and randomly rotate and translate the combinations --> filial generation
              for (size_t iter = 0; iter < max_iter; ++iter){
                std::sort(std::begin(parental), std::end(parental));

                std::vector<Candidate<default_type>> filial;
                auto p1 = std::begin(parental);
                auto p2 = std::begin(parental);
                ++p2;
                while (filial.size() < pool_size && p2 != std::end(parental)) {
                      p1->crossover_rigid(0.2, *p2, filial);
                      filial.back().mutate_rigid(mutation_rad, mutation_t_wrt_fov*spatial_extent);
                      filial.back().run_sgd (metric, transform, im1_smoothed, im2_smoothed, im1_mask, im2_mask);
                      if (filial.back().get_overlap() == 0)
                        filial.pop_back();
                      else
                        ++progress;
                      if (filial.size() == pool_size) break;
                      p1->crossover_rigid(0.8, *p2, filial);
                      filial.back().mutate_rigid(0.2, 0.05*spatial_extent);
                      filial.back().run_sgd (metric, transform, im1_smoothed, im2_smoothed, im1_mask, im2_mask);
                      if (filial.back().get_overlap() == 0)
                        filial.pop_back();
                      else
                        ++progress;
                      ++p1;
                      ++p2;
                }

                for(auto const& f: filial) {
                  if (log_stream) log_os << str(f) << std::endl;
                  geneology.push_back(f);
                }
                if (log_stream) log_os.flush();

                parental = filial;
                if (parental.size() <= 2) break;
              }

              // find specimen with lowest cost but at least median overlap:
              default_type median_overlap;
              std::vector<default_type> overlap_v;
              {
                for(size_t i = 0; i< geneology.size(); ++i) {
                  overlap_v.push_back((default_type) geneology[i].get_overlap());
                }
                median_overlap = Math::median(overlap_v);
              }

              std::sort(std::begin(geneology), std::end(geneology), lower_cost<default_type>);
              auto best = std::find_if (geneology.begin(), geneology.end(), [&median_overlap] (Candidate<default_type> const& c) { return c.get_overlap() >= median_overlap; });

              TrafoType winner = best->get_trafo();
              INFO("global search result: " + str(*best));
              transform.set_transform(winner);

            }

        protected:
          size_t max_iter;
          size_t max_GD_iter;
          default_type scale_factor;
          size_t pool_size;
          default_type mutation_rad;
          default_type mutation_t_wrt_fov;
          default_type loop_density;
          default_type smooth_factor;
          std::vector<size_t> kernel_extent;
          std::streambuf* log_stream;
          size_t gd_repetitions;
      };
    } // namespace GlobalSearch
  }
}

#endif
