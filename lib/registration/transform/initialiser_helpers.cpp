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

#include "registration/transform/initialiser_helpers.h"
#include <Eigen/Geometry>
#include <Eigen/Dense>
#include <Eigen/Eigenvalues>

#include "algo/loop.h"
#include "debug.h"
// #include "timer.h"


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
          std::vector<std::pair<default_type, ssize_t>> eval_idx_vec;
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
          // only use the first volume of a 4D file. This is important for FOD images.
          if (!mask.valid()) {
            for (auto i = Loop (0, 3)(image); i; ++i) {
              m000 += image.value();
            }
          } else {
            for (auto i = Loop (0, 3)(image, mask); i; ++i) {
              if (mask.value() > 0.0)
                m000 += image.value();
            }
          }

          // TODO: multi-threaded loop
          if (!mask.valid()) {
            for (auto i = Loop (0, 3)(image); i; ++i) {
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

        void initialise_using_image_mass (Image<default_type>& im1,
                                          Image<default_type>& im2,
                                          Image<default_type>& mask1,
                                          Image<default_type>& mask2,
                                          Registration::Transform::Base& transform) {
          if (mask1.valid() or mask2.valid()) {
            CONSOLE ("initialising centre of rotation and translation using centre of mass");
          } else {
            CONSOLE ("initialising centre of rotation and translation using unmasked centre of mass");
          }
          Eigen::Matrix<default_type, 3, 1>  im1_centre_of_mass (3);
          im1_centre_of_mass.setZero();
          default_type im1_mass = 0.0;
          MR::Transform im1_transform (im1);
          // only use the first volume of a 4D file. This is important for FOD images.
          if (!mask1.valid()) {
            for (auto i = Loop (0, 3)(im1); i; ++i) {
              Eigen::Vector3 voxel_pos ((default_type)im1.index(0), (default_type)im1.index(1), (default_type)im1.index(2));
              Eigen::Vector3 im1_scanner = im1_transform.voxel2scanner * voxel_pos;
              im1_mass += im1.value();
              im1_centre_of_mass += im1_scanner * im1.value();
            }
          } else {
            for (auto i = Loop (0, 3)(im1, mask1); i; ++i) {
              if (mask1.value()) {
                Eigen::Vector3 voxel_pos ((default_type)im1.index(0), (default_type)im1.index(1), (default_type)im1.index(2));
                Eigen::Vector3 im1_scanner = im1_transform.voxel2scanner * voxel_pos;
                im1_mass += im1.value();
                im1_centre_of_mass += im1_scanner * im1.value();
              }
            }
          }
          im1_centre_of_mass /= im1_mass;

          Eigen::Matrix<default_type, 3, 1> im2_centre_of_mass (3);
          im2_centre_of_mass.setZero();
          default_type im2_mass = 0.0;
          MR::Transform im2_transform (im2);
          if (!mask2.valid()) {
            for (auto i = Loop(0, 3)(im2); i; ++i) {
              Eigen::Vector3 voxel_pos ((default_type)im2.index(0), (default_type)im2.index(1), (default_type)im2.index(2));
              Eigen::Vector3 im2_scanner = im2_transform.voxel2scanner * voxel_pos;
              im2_mass += im2.value();
              im2_centre_of_mass += im2_scanner * im2.value();
            }
          } else {
            for (auto i = Loop(0, 3)(im2, mask2); i; ++i) {
              if (mask2.value()) {
                Eigen::Vector3 voxel_pos ((default_type)im2.index(0), (default_type)im2.index(1), (default_type)im2.index(2));
                Eigen::Vector3 im2_scanner = im2_transform.voxel2scanner * voxel_pos;
                im2_mass += im2.value();
                im2_centre_of_mass += im2_scanner * im2.value();
              }
            }
          }
          im2_centre_of_mass /= im2_mass;

          Eigen::Vector3 centre = (im1_centre_of_mass + im2_centre_of_mass) / 2.0;
          Eigen::Vector3 translation = im1_centre_of_mass - im2_centre_of_mass;
          transform.set_centre_without_transform_update (centre);
          transform.set_translation (translation);
          // VEC(im1_centre_of_mass);
          // VEC(im2_centre_of_mass);
          // VEC(centre);
          // VEC(translation);
          // transform.debug();
        }

        void MomentsInitialiser::run () {
          // Timer timer;
          // timer.start();
          if (!calculate_eigenvectors()) {
            WARN("Image moments not successful. Using centre of mass.");
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
          // MAT(im1_evec);
          // VEC(im1_eval);
          // MAT(im2_evec);
          // VEC(im2_eval);

          // TODO decide whether rotation matrix is sensible
          // Eigen::EigenSolver<Eigen::Matrix<default_type, 3, 3>> es((Eigen::Matrix<default_type, 3, 3>) A);
          // MAT(es.eigenvalues().real().transpose());
          // const Eigen::Matrix<default_type, 3, 3> evec (es.eigenvectors().real().cast<default_type>());
          // MAT(evec);
          // Eigen::Quaternion<default_type, Eigen::AutoAlign> quaternion (evec);
          // std::cerr << "\nw:" << quaternion.w() << " x:" << quaternion.x() << " y:" << quaternion.y() << " z:" << quaternion.z() << "\n";
          // Eigen::Quaternion<default_type, Eigen::AutoAlign> quaternion ((Eigen::Matrix<default_type, 3, 3>) A);
          // std::cerr << "\nw:" << quaternion.w() << " x:" << quaternion.x() << " y:" << quaternion.y() << " z:" << quaternion.z() << "\n";

          // get rotation matrix that aligns all eigenvectors
          auto im1_evec_transpose = im1_evec.transpose().eval();
          auto im2_evec_transpose = im2_evec.transpose().eval();

          Eigen::FullPivHouseholderQR<decltype(im2_evec_transpose)> dec(im2_evec_transpose);
          Eigen::Matrix<default_type, Eigen::Dynamic, Eigen::Dynamic> A (3,3);
          A = dec.solve(im1_evec_transpose);
          assert((A * im1_evec).isApprox(im2_evec));
          assert(std::abs(A.determinant() - 1.0) < 0.0001);
          A = A.transpose().eval(); // A * im2_evec = im1_evec

          // MAT(A);
          // Eigen::Matrix<default_type, 3, 3> A2 (A);
          // Eigen::AngleAxis<default_type> aa(A2);
          // aa.fromRotationMatrix(A);
           // = Eigen::AngleAxis<default_type>(A);
          // VEC(aa.axis());
          // VAR(aa.angle());

          Eigen::Vector3 centre = (im1_centre_of_mass + im2_centre_of_mass) / 2.0;
          Eigen::Vector3 offset = im1_centre_of_mass - im2_centre_of_mass;
          transform.set_centre_without_transform_update (centre);

          Eigen::Translation<default_type, 3> T_offset (offset), T_c2 (im2_centre_of_mass);
          transform_type T, R0;
          R0.setIdentity();
          R0.linear() = A;
          T = T_c2 * T_offset * R0 * T_c2.inverse();
          transform.set_transform(T);

          // MAT(A);
          // VEC(im1_centre_of_mass);
          // VEC(im2_centre_of_mass);
          // VEC(centre);
          // VEC(offset);
          // transform.debug();
          // VAR(timer.elapsed());
        }

        bool MomentsInitialiser::calculate_eigenvectors () {
            default_type  m000, m100, m010, m001, mu110, mu011, mu101, mu200, mu020, mu002;
            get_geometric_centre(im1, im1_centre);
            get_moments (im1, mask1, im1_centre, m000, m100, m010, m001, mu110, mu011, mu101, mu200, mu020, mu002);
            im1_centre_of_mass << m100 / m000, m010 / m000, m001 / m000;
            im1_covariance_matrix(0, 0) = mu200 / m000;
            im1_covariance_matrix(0, 1) = mu110 / m000;
            im1_covariance_matrix(0, 2) = mu101 / m000;
            im1_covariance_matrix(1, 0) = im1_covariance_matrix(0, 1);
            im1_covariance_matrix(1, 1) = mu020 / m000;
            im1_covariance_matrix(1, 2) = mu011 / m000;
            im1_covariance_matrix(2, 0) = im1_covariance_matrix(0, 2);
            im1_covariance_matrix(2, 1) = im1_covariance_matrix(1, 2);
            im1_covariance_matrix(2, 2) = mu002 / m000;

            get_geometric_centre(im2, im2_centre);
            get_moments (im2, mask2, im2_centre, m000, m100, m010, m001, mu110, mu011, mu101, mu200, mu020, mu002);
            im2_centre_of_mass << m100 / m000, m010 / m000, m001 / m000;
            im2_covariance_matrix(0, 0) = mu200 / m000;
            im2_covariance_matrix(0, 1) = mu110 / m000;
            im2_covariance_matrix(0, 2) = mu101 / m000;
            im2_covariance_matrix(1, 0) = im2_covariance_matrix(0, 1);
            im2_covariance_matrix(1, 1) = mu020 / m000;
            im2_covariance_matrix(1, 2) = mu011 / m000;
            im2_covariance_matrix(2, 0) = im2_covariance_matrix(0, 2);
            im2_covariance_matrix(2, 1) = im2_covariance_matrix(1, 2);
            im2_covariance_matrix(2, 2) = mu002 / m000;
            return (get_sorted_eigen_vecs_vals (im2_covariance_matrix, im2_evec, im2_eval) and
                  get_sorted_eigen_vecs_vals (im1_covariance_matrix, im1_evec, im1_eval));
        }
      }
    }
  }
}

