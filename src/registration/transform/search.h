/* Copyright (c) 2008-2019 the MRtrix3 contributors.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Covered Software is provided under this License on an "as is"
 * basis, without warranty of any kind, either expressed, implied, or
 * statutory, including, without limitation, warranties that the
 * Covered Software is free of defects, merchantable, fit for a
 * particular purpose or non-infringing.
 * See the Mozilla Public License v. 2.0 for more details.
 *
 * For more details, see http://www.mrtrix.org/.
 */

#ifndef __registration_transform_search_h__
#define __registration_transform_search_h__

#include <iostream>
#include <Eigen/Geometry>
#include <Eigen/Eigen>

#include "debug.h"
#include "image.h"
#include "progressbar.h"
#include "types.h"

#include "math/math.h"
#include "math/median.h"
#include "math/rng.h"
#include "math/gradient_descent.h"
#include "math/average_space.h"
#include "filter/resize.h"
#include "filter/reslice.h"
#include "adapter/reslice.h"
#include "interp/linear.h"
#include "interp/cubic.h"
#include "interp/nearest.h"
#include "registration/metric/mean_squared.h"
#include "registration/metric/params.h"
// #include "registration/metric/local_cross_correlation.h"
#include "registration/metric/thread_kernel.h"
#include "registration/transform/initialiser.h"
#include "registration/transform/rigid.h"
#include "file/config.h"

namespace MR
{
  namespace Registration
  {
    namespace RotationSearch
    {

      using TrafoType = transform_type;
      using MatType = Eigen::Matrix<default_type, 3, 3>;
      using VecType = Eigen::Matrix<default_type, 3, 1>;
      using QuatType = Eigen::Quaternion<default_type>;

      template <class MetricType = Registration::Metric::MeanSquaredNoGradient>
        class ExhaustiveRotationSearch { MEMALIGN(ExhaustiveRotationSearch<MetricType>)
          public:
            ExhaustiveRotationSearch (
              Image<default_type>& image1,
              Image<default_type>& image2,
              Image<default_type>& mask1,
              Image<default_type>& mask2,
              MetricType& metric_,
              Registration::Transform::Base& linear_transform,
              Registration::Transform::Init::LinearInitialisationParams& init) :
            im1 (image1),
            im2 (image2),
            mask1 (mask1),
            mask2 (mask2),
            metric (metric_),
            input_trafo (linear_transform),
            init_options (init),
            centre (input_trafo.get_centre()),
            offset (input_trafo.get_translation()),
            global_search_iterations (init.init_rotation.search.global.iterations),
            rot_angles (init.init_rotation.search.angles),
            local_search_directions (init.init_rotation.search.directions),
            image_scale_factor (init.init_rotation.search.scale),
            global_search (init.init_rotation.search.run_global),
            translation_extent (init.init_rotation.search.translation_extent),
            idx_angle (0),
            idx_dir (0) {
              local_trafo.set_centre_without_transform_update (centre);
              local_trafo.set_translation (offset);
              Eigen::Matrix<default_type, 3, 3> lin = input_trafo.get_transform().linear();
              local_trafo.set_matrix_const_translation(lin);
              INFO ("before search:");
              INFO (local_trafo.info());
            };


            using ParamType = Metric::Params<Registration::Transform::Rigid,
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
                                     Interp::Nearest<Image<default_type>>>;

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
              const Eigen::Translation<default_type, 3> Tc2 (centre - 0.5 * offset), To (offset);
              transform_type R0;
              R0.translation().fill(0);

              Eigen::Vector3d extent(0,0,0);
              if (translation_extent != 0) {
                ParamType parameters = get_parameters ();
                extent << midway_image_header.spacing(0) * translation_extent * (midway_image_header.size(0) - 0.5),
                                  midway_image_header.spacing(1) * translation_extent * (midway_image_header.size(1) - 0.5),
                                  midway_image_header.spacing(2) * translation_extent * (midway_image_header.size(2) - 0.5);
              }

              while ( ++iteration < iterations ) {
                ++progress;
                if (iteration > 0) {
                  if (global_search)
                    gen_random_quaternion ();
                  else
                    gen_local_quaternion ();

                  R0.linear() = quat.normalized().toRotationMatrix();
                  if (translation_extent != 0) {
                    gen_random_quaternion (); // overwrites quat
                    R0.translation() = rndn () * (quat * extent);
                    DEBUG("translation: " + str(R0.translation().transpose()));
                  }

                  T = Tc2 * To * R0 * Tc2.inverse();
                  local_trafo.set_transform<transform_type>(T);
                }

                ParamType parameters = get_parameters ();
                // parameters.make_diagnostics_image ("/tmp/debugme"+str(iteration)+".mif", true); // REMOVEME
                cost.fill(0);
                cnt = 0;
                Metric::ThreadKernel<MetricType, ParamType> kernel (metric, parameters, cost, gradient, &cnt);
                ThreadedLoop (parameters.midway_image, 0, 3).run (kernel);
                DEBUG ("rotation search: iteration " + str(iteration) + " cost: " + str(cost) + " cnt: " + str(cnt));
                if (debug)
                  std::cout << str(iteration) + " " + str(cost) + " " + str(cnt) << " " << T.matrix().row(0) << " " << T.matrix().row(1) << " " << T.matrix().row(2) << std::endl;
                // write_images ( "im1_" + str(iteration) + ".mif", "im2_" + str(iteration) + ".mif");
                if (cnt == 0) {
                  if (iteration == 0)
                    throw Exception ("zero voxel overlap at initialisation. input matrix wrong?");
                  WARN ("rotation search: overlap count is zero");
                }
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
                // default_type max_overlap = overlap_it.maxCoeff();
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
            FORCE_INLINE ParamType get_parameters () {
              // create resized midway image
              // vector<Eigen::Transform<default_type, 3, Eigen::Projective>> init_transforms;
              // {
              //   Eigen::Transform<default_type, 3, Eigen::Projective> init_trafo_1 = ;
              //   Eigen::Transform<default_type, 3, Eigen::Projective> init_trafo_2 = local_trafo.get_transform_half();
              //   init_transforms.push_back (init_trafo_1);
              //   init_transforms.push_back (init_trafo_2);
              // }
              // auto padding = Eigen::Matrix<default_type, 4, 1>(1.0, 1.0, 1.0, 1.0);
              // int subsample = 1;
              // vector<Header> headers;
              // headers.push_back (Header (im1));
              // headers.push_back (Header (im2));
              midway_image_header = compute_minimum_average_header (im1, im2, local_trafo.get_transform_half_inverse(), local_trafo.get_transform_half());

              Filter::Resize midway_resize_filter (midway_image_header);
              midway_resize_filter.set_scale_factor (image_scale_factor);
              midway_resized_header = Header (midway_resize_filter);

              ParamType parameters (local_trafo, im1, im2, midway_resized_header, mask1, mask2);
              parameters.loop_density = 1.0;
              return parameters;
            }

            // gen_random_quaternion generates random element of SO(3)
            FORCE_INLINE void gen_random_quaternion () {
              // Eigen 3.3.0: quat = Eigen::Quaternion<default_type,Eigen::autoalign>::UnitRandom ();
              // http://planning.cs.uiuc.edu/node198.html
              const default_type u1 = rnd ();
              const default_type u2 = rnd () * 2.0 * Math::pi;
              const default_type u3 = rnd () * 2.0 * Math::pi;
              assert (u1 < 1.0 && u1 >= 0.0);
              assert (u2 < 2.0 * Math::pi && u2 >= 0.0);
              assert (u3 < 2.0 * Math::pi && u3 >= 0.0);
              const default_type a = std::sqrt(1.0 - u1);
              const default_type b = std::sqrt(u1);
              quat = Eigen::Quaternion<default_type> (a * std::sin(u2), a * std::cos(u2), b * std::sin(u3), b * std::cos(u3));
            }

            // gen_uniform_rotation_axes generates roughly uniformly distributed points on sphere
            // starting on z-axis up to -z-axis (max_cone_angle_deg=180). points are stored as matrix
            // of azimuth and elevation
            // ENH: less brute-force approach (for instance: fixed set of electrostatic repulsion directions, rotate all to gap centre)
            FORCE_INLINE void gen_uniform_rotation_axes ( const size_t& n_dir, const default_type& max_cone_angle_deg ) {
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
            FORCE_INLINE void az_el_to_cartesian () {
              xyz.resize (az_el.rows(), 3);
              Eigen::VectorXd el_sin = az_el.col(1).array().sin();
              xyz.col(0).array() = el_sin.array() * az_el.col(0).array().cos();
              xyz.col(1).array() = el_sin.array() * az_el.col(0).array().sin();
              xyz.col(2).array() = az_el.col(1).array().cos();
            }

            FORCE_INLINE void gen_local_quaternion () {
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
            Math::RNG::Uniform<default_type> rnd;
            Eigen::Quaternion<default_type> quat;
            transform_type best_trafo;
            Header midway_image_header;
            default_type min_cost;
            vector<default_type> vec_cost;
            vector<size_t> vec_overlap;
            size_t global_search_iterations;
            vector<default_type> rot_angles;
            size_t local_search_directions;
            default_type image_scale_factor;
            bool global_search;
            double translation_extent;
            size_t idx_angle, idx_dir;
            Registration::Transform::Rigid local_trafo;
            Eigen::Matrix<default_type, Eigen::Dynamic, 2> az_el;
            Eigen::Matrix<default_type, Eigen::Dynamic, 3> xyz;
            Eigen::Matrix<default_type, Eigen::Dynamic, 1> overlap_it, cost_it;
            vector<transform_type> trafo_it;
          };
    } // namespace RotationSearch
  }
}

#endif
