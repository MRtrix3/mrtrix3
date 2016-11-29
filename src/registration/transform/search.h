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

#ifndef __registration_transform_search_h__
#define __registration_transform_search_h__

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
#include "math/average_space.h"
#include "filter/resize.h"
#include "filter/reslice.h"
#include "adapter/reslice.h"
#include "interp/linear.h"
#include "interp/cubic.h"
#include "interp/nearest.h"
#include "registration/metric/mean_squared.h"
#include "registration/metric/params.h"
#include "registration/metric/normalised_cross_correlation.h"
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
    namespace RotationSearch
    {

      typedef transform_type TrafoType;
      typedef Eigen::Matrix<default_type, 3, 3> MatType;
      typedef Eigen::Matrix<default_type, 3, 1> VecType;
      typedef Eigen::Quaternion<default_type> QuatType;

      template <class MetricType = Registration::Metric::MeanSquaredNoGradient>
        class ExhaustiveRotationSearch { MEMALIGN(ExhaustiveRotationSearch<MetricType>)
          public:
            ExhaustiveRotationSearch (
              Image<default_type>& image1,
              Image<default_type>& image2,
              Image<default_type>& mask1,
              Image<default_type>& mask2,
              MetricType& metric,
              Registration::Transform::Base& linear_transform,
              Registration::Transform::Init::LinearInitialisationParams& init) :
            im1 (image1),
            im2 (image2),
            mask1 (mask1),
            mask2 (mask2),
            metric (metric),
            input_trafo (linear_transform),
            init_options (init),
            centre (input_trafo.get_centre()),
            offset (input_trafo.get_translation()),
            global_search_iterations (init.init_rotation.search.global.iterations),
            rot_angles (init.init_rotation.search.angles),
            local_search_directions (init.init_rotation.search.directions),
            image_scale_factor (init.init_rotation.search.scale),
            global_search (init.init_rotation.search.run_global),
            idx_angle (0),
            idx_dir (0) {
              local_trafo.set_centre_without_transform_update (centre);
              local_trafo.set_translation (offset);
            };


            typedef Metric::Params<Registration::Transform::Rigid,
                                     Image<default_type>,
                                     Image<default_type>,
                                     Header,
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
              image1_midway_header.ndim() = im1.ndim();
              for (size_t dim = 3; dim < im1.ndim(); ++dim){
                image1_midway_header.spacing(dim) = im1.spacing(dim);
                image1_midway_header.size(dim) = im1.size(dim);
              }
              image1_midway = Image<default_type>::create (im1_path, image1_midway_header);
              Header image2_midway_header (midway_image_header);
              image2_midway_header.datatype() = DataType::Float64;
              image2_midway_header.ndim() = im2.ndim();
              for (size_t dim = 3; dim < im2.ndim(); ++dim){
                image2_midway_header.spacing(dim) = im2.spacing(dim);
                image2_midway_header.size(dim) = im2.size(dim);
              }
              image2_midway = Image<default_type>::create (im2_path, image2_midway_header);

              Filter::reslice<Interp::Cubic> (im1, image1_midway, local_trafo.get_transform_half(), Adapter::AutoOverSample, 0.0);
              Filter::reslice<Interp::Cubic> (im2, image2_midway, local_trafo.get_transform_half_inverse(), Adapter::AutoOverSample, 0.0);
            }

            void run ( bool debug = false ) {

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

              size_t iteration (0);
              ssize_t cnt (0);
              Eigen::Matrix<default_type, Eigen::Dynamic, 1> gradient (local_trafo.size());
              Eigen::VectorXd cost = Eigen::VectorXd::Zero(1,1);
              transform_type T;
              {
                ParamType parameters = get_parameters ();
                Metric::ThreadKernel<MetricType, ParamType> kernel (metric, parameters, cost, gradient, &cnt);
                ThreadedLoop (parameters.midway_image, 0, 3).run (kernel);
                assert (cnt > 0);
                overlap_it[0] = cnt;
                cost_it[0] = cost(0) / static_cast<default_type>(cnt);
                T = parameters.transformation.get_transform();
                trafo_it.push_back (T);
              }

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
                transform_type T = Tc2 * To * R0 * Tc2.inverse();
                local_trafo.set_transform<decltype(T)>(T);
                ParamType parameters = get_parameters ();
                // parameters.transformation.set_transform<decltype(T)>(T);
                cost.fill(0);
                cnt = 0;
                Metric::ThreadKernel<MetricType, ParamType> kernel (metric, parameters, cost, gradient, &cnt);
                ThreadedLoop (parameters.midway_image, 0, 3).run (kernel);
                DEBUG ("rotation search: iteration " + str(iteration) + " cost: " + str(cost) + " cnt: " + str(cnt));
                // write_images ( "im1_" + str(iteration) + ".mif", "im2_" + str(iteration) + ".mif");
                overlap_it[iteration] = cnt;
                cost_it[iteration] = cost(0) / static_cast<default_type>(cnt);
                trafo_it.push_back (T);
              }
              // if (debug) {
              //   save_matrix(cost_it, "/tmp/cost_before.txt");
              //   save_matrix(overlap_it, "/tmp/overlap.txt");
              // }
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
              // if (debug) {
              //   save_matrix(cost_it, "/tmp/cost_after.txt");
              //   Eigen::VectorXd t(2);
              //   t(0) = cost_it(0);
              //   t(1) = min_cost;
              //   save_matrix(t, "/tmp/cost_mass_chosen.txt");
              //   save_matrix(centre, "/tmp/centre.txt");
              //   parameters.transformation.set_transform (best_trafo);
              //   write_images ( "/tmp/im1_best.mif", "/tmp/im2_best.mif");
              // }
              input_trafo.set_transform<transform_type> (best_trafo);

            };

          private:
            ParamType get_parameters () {
              // create resized midway image
              memalign_vector<Eigen::Transform<default_type, 3, Eigen::Projective>>::type init_transforms;
              {
                Eigen::Transform<default_type, 3, Eigen::Projective> init_trafo_1 = local_trafo.get_transform_half_inverse();
                Eigen::Transform<default_type, 3, Eigen::Projective> init_trafo_2 = local_trafo.get_transform_half();
                init_transforms.push_back (init_trafo_1);
                init_transforms.push_back (init_trafo_2);
              }
              auto padding = Eigen::Matrix<default_type, 4, 1>(0.0, 0.0, 0.0, 0.0);
              int subsample = 1;
              memalign_vector<Header>::type headers;
              headers.push_back (Header (im1));
              headers.push_back (Header (im2));
              midway_image_header = compute_minimum_average_header (headers, subsample, padding, init_transforms);

              Filter::Resize midway_resize_filter (midway_image_header);
              midway_resize_filter.set_scale_factor (image_scale_factor);
              midway_resized_header = Header (midway_resize_filter);

              ParamType parameters (local_trafo, im1, im2, midway_resized_header, mask1, mask2);
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
            Header midway_resized_header;
            MetricType metric;
            Registration::Transform::Base& input_trafo;
            Registration::Transform::Init::LinearInitialisationParams& init_options;
            const Eigen::Vector3d centre;
            const Eigen::Vector3d offset;
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
            default_type image_scale_factor;
            bool global_search;
            size_t idx_angle, idx_dir;
            Registration::Transform::Rigid local_trafo;
            Eigen::Matrix<default_type, Eigen::Dynamic, 2> az_el;
            Eigen::Matrix<default_type, Eigen::Dynamic, 3> xyz;
            Eigen::Matrix<default_type, Eigen::Dynamic, 1> overlap_it, cost_it;
            memalign_vector<transform_type>::type trafo_it;
          };
    } // namespace RotationSearch
  }
}

#endif
