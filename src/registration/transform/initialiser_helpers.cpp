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

#include <Eigen/Geometry>
#include <Eigen/Dense>
#include <Eigen/Eigenvalues>
#include "registration/transform/initialiser_helpers.h"
#include "registration/transform/search.h"
#include "registration/multi_resolution_lmax.h"
#include "registration/multi_contrast.h"

#include "algo/loop.h"
#include "algo/threaded_loop.h"
#include "debug.h"
// #define DEBUG_INIT

namespace MR
{
  namespace Registration
  {
    namespace Transform
    {
      namespace Init
      {
        bool get_sorted_eigen_vecs_vals (const Eigen::Matrix<default_type, 3, 3>& mat,
          Eigen::Matrix<default_type, Eigen::Dynamic, Eigen::Dynamic>& eigenvectors,
          Eigen::Matrix<default_type, Eigen::Dynamic, 1>& eigenvals) {
          Eigen::EigenSolver<Eigen::Matrix<default_type, 3, 3>> es(mat);
          if (es.info() != Eigen::Success)
            return false;
          const Eigen::Matrix<default_type, Eigen::Dynamic, Eigen::Dynamic>& evec = es.eigenvectors().real();
          const Eigen::Matrix<default_type, Eigen::Dynamic, 1> eval (es.eigenvalues().real()); // copy on purpose
          assert(evec.cols() == eval.size());

          eigenvectors.resize(evec.rows(), evec.cols());
          eigenvals.resize(eval.size());

          // sort eigenvectors by eigenvalue, largest first
          vector<std::pair<default_type, ssize_t>> eval_idx_vec;
          for(ssize_t i = 0; i < eval.size(); ++i ) {
              eval_idx_vec.emplace_back(eval[i], i);
          }
          std::sort( std::begin(eval_idx_vec), std::end(eval_idx_vec), std::greater<std::pair<default_type, ssize_t>>() );

          for(ssize_t i = 0; i < eval.size(); ++i ) {
            eigenvals[i] = eval_idx_vec[i].first;
            eigenvectors.col(i) = evec.col(eval_idx_vec[i].second);
          }
          return true;
        }

        template <class ImageType, class MaskType>
          class WeightedMomentsFunctor { MEMALIGN(WeightedMomentsFunctor)
          public:
            WeightedMomentsFunctor (const ImageType& image,
                                    const MaskType& mask,
                                    const Eigen::Matrix<default_type, 3, 1>& centre,
                                    Eigen::VectorXd& weighted_m,
                                    Eigen::VectorXd& weighted_mu,
                                    const vector<MultiContrastSetting>& contrast_settings) :
              transform (image), mask (mask),
              centre (centre),
              global_m (weighted_m), global_mu (weighted_mu) {
                local_m.resize(4);
                local_m.setZero();
                local_mu.resize(6);
                local_mu.setZero();
                start_vol.resize (std::max(contrast_settings.size(), size_t(1)), 0);
                weight.resize (std::max(contrast_settings.size(), size_t(1)), 1.0);
                for (size_t idx = 0; idx < contrast_settings.size(); idx++) {
                  start_vol[idx] = contrast_settings[idx].start;
                  weight[idx] = contrast_settings[idx].weight;
                }
              }

            ~WeightedMomentsFunctor () {
              global_m += local_m;
              global_mu += local_mu;
            }

            template <class ImType>
              void operator() (ImType& image) {
                if (mask.valid()) {
                  assign_pos_of (image, 0, 3). to (mask);
                  if (!mask.value())
                    return;
                }
                Eigen::Vector3 voxel_pos ((default_type)image.index(0), (default_type)image.index(1), (default_type)image.index(2));
                Eigen::Vector3 scanner_pos = transform.voxel2scanner * voxel_pos;

                default_type xc = scanner_pos[0] - centre[0];
                default_type yc = scanner_pos[1] - centre[1];
                default_type zc = scanner_pos[2] - centre[2];

                for (size_t idx = 0; idx < start_vol.size(); idx++) {
                  if (image.ndim() > 3)
                    image.index(3) = start_vol[idx];

                  const default_type val = weight[idx] * image.value();
                  if (std::isfinite (val)) {

                    // m = [m000, m100, m010, m001]
                    // Eigen::VectorXd mu = [mu110, mu011, mu101, mu200, mu020, mu002]
                    local_m[0] += val;

                    local_m[1] += scanner_pos[0] * val;
                    local_m[2] += scanner_pos[1] * val;
                    local_m[3] += scanner_pos[2] * val;

                    local_mu[0] += xc * yc * val;
                    local_mu[1] += yc * zc * val;
                    local_mu[2] += xc * zc * val;

                    local_mu[3] += Math::pow2(xc) * val;
                    local_mu[4] += Math::pow2(yc) * val;
                    local_mu[5] += Math::pow2(zc) * val;
                  }
                }
                if (image.ndim() > 3)
                  image.index(3) = 0;
              }

          protected:
            MR::Transform transform;
            MaskType mask;
            const Eigen::Matrix<default_type, 3, 1> centre;
            Eigen::VectorXd& global_m;
            Eigen::VectorXd& global_mu;
            Eigen::VectorXd local_m;
            Eigen::VectorXd local_mu;
            vector<size_t> start_vol;
            vector<default_type> weight;
            Eigen::Vector3d voxel_pos, scanner_pos;
          };

        void get_moments (Image<default_type>& image,
                          Image<default_type>& mask,
                          const Eigen::Matrix<default_type, 3, 1>& centre,
                          default_type& m000,
                          default_type& m100,
                          default_type& m010,
                          default_type& m001,
                          default_type& mu110,
                          default_type& mu011,
                          default_type& mu101,
                          default_type& mu200,
                          default_type& mu020,
                          default_type& mu002) {
          m000 = 0.0; m100 = 0.0; m010 = 0.0; m001 = 0.0; mu110 = 0.0; mu011 = 0.0;
          mu101 = 0.0; mu200 = 0.0; mu020 = 0.0; mu002 = 0.0;
          MR::Transform transform (image);

          if (!mask.valid()) {
            for (auto i = Loop (0, 3)(image); i; ++i) {
              Eigen::Vector3 voxel_pos ((default_type)image.index(0), (default_type)image.index(1), (default_type)image.index(2));
              Eigen::Vector3 scanner_pos = transform.voxel2scanner * voxel_pos;

              default_type xc = scanner_pos[0] - centre[0];
              default_type yc = scanner_pos[1] - centre[1];
              default_type zc = scanner_pos[2] - centre[2];

              default_type val = image.value();
              m000 += val;

              m100 += scanner_pos[0] * val;
              m010 += scanner_pos[1] * val;
              m001 += scanner_pos[2] * val;

              mu110 += xc * yc * val;
              mu011 += yc * zc * val;
              mu101 += xc * zc * val;

              mu200 += Math::pow2(xc) * val;
              mu020 += Math::pow2(yc) * val;
              mu002 += Math::pow2(zc) * val;
            }
          } else {
            for (auto i = Loop (0, 3)(image, mask); i; ++i) {
              if (mask.value() <= 0.0)
                continue;
              Eigen::Vector3 voxel_pos ((default_type)image.index(0), (default_type)image.index(1), (default_type)image.index(2));
              Eigen::Vector3 scanner_pos = transform.voxel2scanner * voxel_pos;
              default_type val = image.value();

              default_type xc = scanner_pos[0] - centre[0];
              default_type yc = scanner_pos[1] - centre[1];
              default_type zc = scanner_pos[2] - centre[2];

              m100 += scanner_pos[0] * val;
              m010 += scanner_pos[1] * val;
              m001 += scanner_pos[2] * val;

              mu110 += xc * yc * val;
              mu011 += yc * zc * val;
              mu101 += xc * zc * val;

              mu200 += Math::pow2(xc) * val;
              mu020 += Math::pow2(yc) * val;
              mu002 += Math::pow2(zc) * val;
            }
          }
        }

        template <class ImageType, class MaskType>
        class WeightedMassFunctor { MEMALIGN(WeightedMassFunctor)
        public:
          WeightedMassFunctor (const ImageType& image,
            const MaskType& mask,
            default_type& weighted_mass,
            Eigen::Vector3d& weighted_centre_of_mass,
            const vector<MultiContrastSetting>& contrast_settings) :
            transform (image), mask (mask), mass (0.0), global_mass (weighted_mass), global_centre_of_mass (weighted_centre_of_mass) {
              centre_of_mass.setZero();
              start_vol.resize (std::max(contrast_settings.size(), size_t(1)), 0);
              weight.resize (std::max(contrast_settings.size(), size_t(1)), 1.0);
              for (size_t idx = 0; idx < contrast_settings.size(); idx++) {
                start_vol[idx] = contrast_settings[idx].start;
                weight[idx] = contrast_settings[idx].weight;
              }
            }

          ~WeightedMassFunctor () {
            global_mass += mass;
            global_centre_of_mass += centre_of_mass;
          }

          template <class ImType>
            void operator() (ImType& image) {
              if (mask.valid()) {
                assign_pos_of (image, 0 , 3). to (mask);
                if (!mask.value())
                  return;
              }
              voxel_pos << (default_type)image.index(0), (default_type)image.index(1), (default_type)image.index(2);
              scanner = transform.voxel2scanner * voxel_pos;
              for (size_t idx = 0; idx < start_vol.size(); idx++) {
                if (image.ndim() > 3)
                  image.index(3) = start_vol[idx];
                if (std::isfinite ((default_type)image.value())) {
                  mass += weight[idx] * image.value();
                  centre_of_mass += scanner * (image.value() * weight[idx]);
                }
              }
              if (image.ndim() > 3)
                image.index(3) = 0;
            }

        protected:
          MR::Transform transform;
          MaskType mask;
          default_type mass;
          default_type& global_mass;
          Eigen::Vector3d& global_centre_of_mass;
          Eigen::Vector3d centre_of_mass;
          vector<size_t> start_vol;
          vector<default_type> weight;
          Eigen::Vector3d voxel_pos, scanner;
        };

        void get_centre_of_mass (Image<default_type>& im,
                                 Image<default_type>& mask,
                                 Eigen::Vector3& centre_of_mass,
                                 const vector<MultiContrastSetting>& contrast_settings) {
          centre_of_mass.setZero();
          default_type mass (0.0);

          ThreadedLoop (im, 0, 3, 2).run (WeightedMassFunctor<Image<default_type>, Image<default_type>> (im, mask, mass, centre_of_mass, contrast_settings), im);

          if (mass == default_type(0.0))
            throw Exception("centre of mass initialisation not possible for empty image");
          centre_of_mass /= mass;
          DEBUG ("centre of mass of " + im.name() + ": " + str(centre_of_mass.transpose()));
        }

        void initialise_using_rotation_search (
          Image<default_type>& im1,
          Image<default_type>& im2,
          Image<default_type>& mask1,
          Image<default_type>& mask2,
          Registration::Transform::Base& transform,
          Registration::Transform::Init::LinearInitialisationParams& init,
          const vector<MultiContrastSetting>& contrast_settings) {

          CONSOLE ("searching for best rotation");
          Registration::Metric::MeanSquaredNoGradient metric; // replace with CrossCorrelationNoGradient?
          Image<default_type> bogus_mask;
          if (contrast_settings.size()>1)
            WARN ("rotation search does not support multiple contrasts. using only first volume of first contrasts");

          RotationSearch::ExhaustiveRotationSearch<decltype(metric)> (
            im1, im2,
            init.init_translation.unmasked1 ? bogus_mask : mask1,
            init.init_translation.unmasked2 ? bogus_mask : mask2,
            metric,
            transform,
            init).run (false);
        }

        void initialise_using_image_mass (Image<default_type>& im1,
                                          Image<default_type>& im2,
                                          Image<default_type>& mask1,
                                          Image<default_type>& mask2,
                                          Registration::Transform::Base& transform,
                                          Registration::Transform::Init::LinearInitialisationParams& init,
                                          const vector<MultiContrastSetting>& contrast_settings) {

          CONSOLE ("initialising translation and centre of rotation using centre of mass");
          Image<default_type> bogus_mask;
          Eigen::Matrix<default_type, 3, 1> im1_centre_of_mass, im2_centre_of_mass;

          // ENH: add option to use mask instead of image intensities

          get_centre_of_mass (im1, init.init_translation.unmasked1 ? bogus_mask : mask1, im1_centre_of_mass, contrast_settings);
          get_centre_of_mass (im2, init.init_translation.unmasked2 ? bogus_mask : mask2, im2_centre_of_mass, contrast_settings);

          Eigen::Vector3 centre = (im1_centre_of_mass + im2_centre_of_mass) / 2.0;
          Eigen::Vector3 translation = im1_centre_of_mass - im2_centre_of_mass;
          transform.set_centre_without_transform_update (centre);
          transform.set_translation (translation);
#ifdef DEBUG_INIT
          VEC(im1_centre_of_mass);
          VEC(im2_centre_of_mass);
          VEC(centre);
          VEC(translation);
          transform.debug();
#endif
        }
#ifdef DEBUG_INIT
        // for debug only:
        void MomentsInitialiser::create_moments_images () {
          std::string f1 = im1.name();
          std::string f2 = im2.name();
          size_t found = f1.rfind(".");
          if (found==std::string::npos)
            throw Exception ("problem here 1");
          f1 = f1.substr(0,found).append("_ev.mif");

          found = f2.rfind(".");
          if (found==std::string::npos)
            throw Exception ("problem here 2");
          f2 = f2.substr(0,found).append("_ev.mif");

          Header new_header1, new_header2;
          new_header1.ndim() = 4;
          new_header2.ndim() = 4;
          for (ssize_t dim=0; dim < 3; ++dim){
            new_header1.size(dim) = im1.size(dim);
            new_header2.size(dim) = im2.size(dim);
            new_header1.spacing(dim) = im1.spacing(dim);
            new_header2.spacing(dim) = im2.spacing(dim);
          }
          new_header1.transform() = im1.transform();
          new_header2.transform() = im2.transform();
          new_header1.size(3) = 9;
          new_header2.size(3) = 9;
          new_header1.spacing(3) = 1;
          new_header2.spacing(3) = 1;

          Image<default_type> im1_moments = Image<default_type>::create(f1, new_header1);
          Image<default_type> im2_moments = Image<default_type>::create(f2, new_header2);

          // Transform tra1;
          MR::Transform T1 (im1);
          MR::Transform T2 (im2);
          Eigen::Vector3 c1 = T1.scanner2voxel * im1_centre_of_mass;
          Eigen::Vector3 c2 = T2.scanner2voxel * im2_centre_of_mass;
          VEC(c1)
          VEC(c2)
          im1_moments.index(0) = std::round(c1[0]);
          im1_moments.index(1) = std::round(c1[1]);
          im1_moments.index(2) = std::round(c1[2]);
          im2_moments.index(0) = std::round(c2[0]);
          im2_moments.index(1) = std::round(c2[1]);
          im2_moments.index(2) = std::round(c2[2]);
          for (ssize_t i=0; i<3; i++) {
            for (ssize_t j=0; j<3; j++) {
              im1_moments.value() = im1_evec(j,i);
              im2_moments.value() = im2_evec(j,i);
              if (i*j<9) im1_moments.index(3)++;
              if (i*j<9) im2_moments.index(3)++;
            }
          }
          // im2_moments.value() = 1;
          MAT(im1_covariance_matrix);
          MAT(im2_covariance_matrix);
          // im1_centre_of_mass, im2_centre_of_mass;
          // im1_covariance_matrix, im2_covariance_matrix;
          // im1_evec, im2_evec;
        }
#endif

        void MomentsInitialiser::run () {
          if (!calculate_eigenvectors(im1, im2, mask1, mask2)) {
            WARN ("Image moments not successful. Using centre of mass instead.");
            Eigen::Vector3 centre = (im1_centre_of_mass + im2_centre_of_mass) / 2.0;
            transform.set_centre (centre);
            Eigen::Vector3 translation = im1_centre_of_mass - im2_centre_of_mass;
            transform.set_translation (translation);
            return;
          }

          assert(im1_evec.col(0).transpose() * im1_evec.col(1) < 0.0001);
          assert(im1_evec.col(1).transpose() * im1_evec.col(2) < 0.0001);
          assert(im1_evec.col(0).transpose() * im1_evec.col(2) < 0.0001);
          assert(im2_evec.col(0).transpose() * im2_evec.col(1) < 0.0001);
          assert(im2_evec.col(1).transpose() * im2_evec.col(2) < 0.0001);
          assert(im2_evec.col(0).transpose() * im2_evec.col(2) < 0.0001);

          // TODO decide whether rotation is sensible using covariance eigenvalues
          // Maaref, A., & Aïssa, S. (2007). Eigenvalue distributions of wishart-type random matrices with application to the performance analysis of MIMO MRC systems. IEEE Transactions on Wireless Communications, 6(7), 2678–2689. doi:10.1109/TWC.2007.05990
          // Edelman, A. (1991). The distribution and moments of the smallest eigenvalue of a random matrix of wishart type. Linear Algebra and Its Applications, 159, 55–80. doi:10.1016/0024-3795(91)90076-9
          // CONSOLE("before permutation");
          // MAT(im1_evec);
          // VEC(im1_eval);
          // MAT(im2_evec);
          // VEC(im2_eval);

          // permute eigenvectors to find maximum sum of (absolute) dot products between the
          // eigenvectors of image 1 and image 2
          // Eigen::PermutationMatrix<Eigen::Dynamic, Eigen::Dynamic> perm(3);
          // perm.setIdentity();
          // Eigen::PermutationMatrix<Eigen::Dynamic, Eigen::Dynamic> bestperm(3);
          // bestperm.setIdentity();
          // default_type bestdotprod = (im2_evec.array() * im1_evec.array()).colwise().sum().abs().sum();
          // while (std::next_permutation(perm.indices().data(), perm.indices().data()+perm.indices().size())) {
          //   auto im2_evec_perm = im2_evec * perm; // permute columns
          //   default_type dotprod = (im2_evec_perm.array() * im1_evec.array()).colwise().sum().abs().sum();
          //   if (dotprod > bestdotprod) bestperm = perm;
          // }
          // im2_evec = (im2_evec * bestperm).eval();
          // im2_eval = (im2_eval * bestperm).eval();

          // flip eigenvectors to same handedness
          im2_evec.array().rowwise() *= (im2_evec.array() * im1_evec.array()).colwise().sum()
            .cwiseQuotient((im2_evec.array() * im1_evec.array()).colwise().sum().abs()).eval();

          // CONSOLE("after permutation");
#ifdef DEBUG_INIT
          MAT(im1_evec);
          VEC(im1_eval);
          MAT(im2_evec);
          VEC(im2_eval);
#endif

          // get rotation matrix that aligns all eigenvectors
          auto im1_evec_transpose = im1_evec.transpose().eval();
          auto im2_evec_transpose = im2_evec.transpose().eval();

          Eigen::FullPivHouseholderQR<decltype(im2_evec_transpose)> dec(im2_evec_transpose);
          Eigen::Matrix<default_type, Eigen::Dynamic, Eigen::Dynamic> A (3,3);
          A = dec.solve(im1_evec_transpose);
          assert((A * im1_evec).isApprox(im2_evec));
          assert(abs(A.determinant() - 1.0) < 0.0001);
          A = A.transpose().eval(); // A * im2_evec = im1_evec

          Eigen::Vector3 centre = (im1_centre_of_mass + im2_centre_of_mass) / 2.0;
          Eigen::Vector3 offset = im1_centre_of_mass - im2_centre_of_mass;
          transform.set_centre_without_transform_update (centre);

          Eigen::Translation<default_type, 3> T_offset (offset), T_c2 (im2_centre_of_mass);
          transform_type T, R0;
          R0.setIdentity();
          R0.linear() = A;
          T = T_c2 * T_offset * R0 * T_c2.inverse();
          transform.set_transform(T);
#ifdef DEBUG_INIT
          MAT(A);
          VEC(centre);
          VEC(offset);
          VEC(im1_centre_of_mass);
          VEC(im2_centre_of_mass);
          transform.debug();
#endif
        }

        bool MomentsInitialiser::calculate_eigenvectors (
          Image<default_type>& image_1,
          Image<default_type>& image_2,
          Image<default_type>& mask_1,
          Image<default_type>& mask_2) {
            Eigen::VectorXd m = Eigen::VectorXd::Zero (4); // m000, m100, m010, m001
            Eigen::VectorXd mu = Eigen::VectorXd::Zero (6); // mu110, mu011, mu101, mu200, mu020, mu002
            get_geometric_centre (image_1, im1_centre);
            ThreadedLoop (image_1, 0, 3, 2).run (WeightedMomentsFunctor<Image<default_type>, Image<default_type>> (image_1, mask_1, im1_centre, m, mu, contrast_settings), image_1);
            im1_centre_of_mass << m[1] / m[0], m[2] / m[0], m[3] / m[0];
            im1_covariance_matrix(0, 0) = mu[3] / m[0];
            im1_covariance_matrix(0, 1) = mu[0] / m[0];
            im1_covariance_matrix(0, 2) = mu[2] / m[0];
            im1_covariance_matrix(1, 0) = im1_covariance_matrix(0, 1);
            im1_covariance_matrix(1, 1) = mu[4] / m[0];
            im1_covariance_matrix(1, 2) = mu[1] / m[0];
            im1_covariance_matrix(2, 0) = im1_covariance_matrix(0, 2);
            im1_covariance_matrix(2, 1) = im1_covariance_matrix(1, 2);
            im1_covariance_matrix(2, 2) = mu[5] / m[0];

            get_geometric_centre (image_2, im2_centre);
            m.setZero();
            mu.setZero();
            ThreadedLoop (image_2, 0, 3, 2).run (WeightedMomentsFunctor<Image<default_type>, Image<default_type>> (image_2, mask_2, im2_centre, m, mu, contrast_settings), image_2);
            im2_centre_of_mass << m[1] / m[0], m[2] / m[0], m[3] / m[0];
            im2_covariance_matrix(0, 0) = mu[3] / m[0];
            im2_covariance_matrix(0, 1) = mu[0] / m[0];
            im2_covariance_matrix(0, 2) = mu[2] / m[0];
            im2_covariance_matrix(1, 0) = im2_covariance_matrix(0, 1);
            im2_covariance_matrix(1, 1) = mu[4] / m[0];
            im2_covariance_matrix(1, 2) = mu[1] / m[0];
            im2_covariance_matrix(2, 0) = im2_covariance_matrix(0, 2);
            im2_covariance_matrix(2, 1) = im2_covariance_matrix(1, 2);
            im2_covariance_matrix(2, 2) = mu[5] / m[0];
            return (get_sorted_eigen_vecs_vals (im2_covariance_matrix, im2_evec, im2_eval) and
                  get_sorted_eigen_vecs_vals (im1_covariance_matrix, im1_evec, im1_eval));
        }

        void FODInitialiser::init (Image<default_type>& im,
          Image<default_type>& mask,
          Eigen::Matrix<default_type, Eigen::Dynamic, 1>& sh,
          Eigen::Matrix<default_type, 3, 1>& centre_of_mass) {
          // centre of mass is calculated using only the zeroth order
          // TODO multithread
          Eigen::Vector3  voxel_pos = Eigen::Vector3::Zero();
          Eigen::Vector3  scanner = Eigen::Vector3::Zero();
          MR::Transform im_transform (im);
          default_type im_mass (0.0);

          size_t cnt (0);
          if (mask.valid()) {
            for (auto i = Loop (0, 3)(im, mask); i; ++i) {
              if (mask.value()) {
                im_mass += im.value();
                voxel_pos << (default_type)im.index(0), (default_type)im.index(1), (default_type)im.index(2);
                scanner = im_transform.voxel2scanner * voxel_pos;
                centre_of_mass += scanner * im.value();
                for (im.index(3) = 0; im.index(3) < N; ++im.index(3))
                  sh[im.index(3)] += im.value();
                ++cnt;
              }
            }
          } else { // no mask
            for (auto i = Loop (0, 3)(im); i; ++i) {
              im_mass += im.value();
              voxel_pos << (default_type)im.index(0), (default_type)im.index(1), (default_type)im.index(2);
              scanner = im_transform.voxel2scanner * voxel_pos;
              centre_of_mass += scanner * im.value();
              for (im.index(3) = 0; im.index(3) < N; ++im.index(3))
                sh[im.index(3)] += im.value();
            }
            cnt = im.size(0) * im.size(1) * im.size(2);
          }
          if (cnt == 0) throw Exception ("empty mask");
          sh /= default_type(cnt);
          centre_of_mass /= im_mass;
        }

        void FODInitialiser::run () {
          FODInitialiser::init (im1, mask1, sh1, im1_centre_of_mass);
          FODInitialiser::init (im2, mask2, sh2, im2_centre_of_mass);
          transform.set_centre (0.5*(im1_centre_of_mass + im2_centre_of_mass));
          transform.set_translation (0.5*(im1_centre_of_mass-im2_centre_of_mass));
          // TODO: rotation
#ifdef DEBUG_INIT
          VEC(im1_centre_of_mass);
          VEC(im2_centre_of_mass);
          VEC(sh1);
          VEC(sh2);
#endif
        }

      }
    }
  }
}

