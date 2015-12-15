/*
 Copyright 2012 Brain Research Institute, Melbourne, Australia

 Written by David Raffelt, 23/02/2012.

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


#ifndef __registration_transform_initialiser_h__
#define __registration_transform_initialiser_h__

#include "transform.h"
#include "algo/loop.h"
#include <algorithm>
#include <Eigen/Eigenvalues>
#include <Eigen/Geometry>
#include <Eigen/Dense>
#include "timer.h"

namespace MR
{
  namespace Registration
  {
    namespace Transform
    {
      namespace Init
      {
        enum InitType {mass, geometric, moments, linear_from_file, none};

        template <class ImageType, class ValueType>
          void get_geometric_centre (const ImageType& image, Eigen::Matrix<ValueType, 3, 1>& centre)
          {
              Eigen::Vector3 centre_voxel;
              centre_voxel[0] = (static_cast<default_type>(image.size(0)) / 2.0) - 1.0;
              centre_voxel[1] = (static_cast<default_type>(image.size(1)) / 2.0) - 1.0;
              centre_voxel[2] = (static_cast<default_type>(image.size(2)) / 2.0) - 1.0;
              MR::Transform transform (image);
              centre  = transform.voxel2scanner * centre_voxel;
          }

        template <class MatrixType, class ValueType = default_type>
          bool get_sorted_eigen_vecs_vals (const MatrixType& mat, Eigen::Matrix<ValueType, Eigen::Dynamic, Eigen::Dynamic>& eigenvectors, Eigen::Matrix<ValueType, Eigen::Dynamic, 1>& eigenvals)
          {
            Eigen::EigenSolver<MatrixType> es(mat);
            if (es.info() != Eigen::Success) {
              return false;
            }
            Eigen::Matrix<ValueType, Eigen::Dynamic, 1> eval;
            const Eigen::Matrix<ValueType, Eigen::Dynamic, Eigen::Dynamic>& evec = es.eigenvectors().real();
            eval = es.eigenvalues().real(); // copy on purpose
            assert(evec.cols() == eval.size());

            eigenvectors.resize(evec.rows(), evec.cols());
            eigenvectors.setZero();

            eigenvals.resize(eval.size());
            eigenvals.setZero();

            // sort eigenvectors by eigenvalue
            std::vector<std::pair<decltype(eval[0]), ssize_t>> eval_idx_vec;
            for(ssize_t i = 0; i < eval.size(); ++i ) {
                eval_idx_vec.emplace_back(eval[i], i);
            }
            std::sort( std::begin(eval_idx_vec), std::end(eval_idx_vec), std::greater<std::pair<decltype(eval[0]), ssize_t>>() );

            for(ssize_t i = 0; i < eval.size(); ++i ) {
              eigenvals[i] = eval_idx_vec[i].first;
              eigenvectors.col(i) = evec.col(eval_idx_vec[i].second);
            }
            VAR(eval.transpose());
            VAR(eigenvals.transpose());
            return true;
          }

        template <class Im1ImageType,
                  class Im2ImageType,
                  class TransformType>
          void initialise_using_image_centres (const Im1ImageType& im1,
                                               const Im2ImageType& im2,
                                               TransformType& transform)
          {
            CONSOLE ("initialising centre of rotation and translation using geometric centre");
            Eigen::Vector3 im1_centre_scanner;
            get_geometric_centre(im1, im1_centre_scanner);

            Eigen::Vector3 im2_centre_scanner;
            get_geometric_centre(im2, im2_centre_scanner);

            Eigen::Vector3 translation = im1_centre_scanner - im2_centre_scanner;
            Eigen::Vector3 centre = (im1_centre_scanner + im2_centre_scanner) / 2.0;
            transform.set_centre (centre);
            transform.set_translation (translation);
          }

        template <class ImageType, class ValueType>
          void get_moments (ImageType& image,
                            const Eigen::Matrix<ValueType, 3, 1>& centre,
                            ValueType& m000,
                            ValueType& m100,
                            ValueType& m010,
                            ValueType& m001,
                            ValueType& mu110,
                            ValueType& mu011,
                            ValueType& mu101,
                            ValueType& mu200,
                            ValueType& mu020,
                            ValueType& mu002)
          {
            m000 = 0.0; m100 = 0.0; m010 = 0.0; m001 = 0.0; mu110 = 0.0; mu011 = 0.0;
            mu101 = 0.0; mu200 = 0.0; mu020 = 0.0; mu002 = 0.0;
            MR::Transform transform (image);
            // only use the first volume of a 4D file. This is important for FOD images.
            for (auto i = Loop (0, 3)(image); i; ++i) {
              m000 += image.value();
            }
            for (auto i = Loop (0, 3)(image); i; ++i) {
              Eigen::Vector3 voxel_pos ((default_type)image.index(0), (default_type)image.index(1), (default_type)image.index(2));
              Eigen::Vector3 scanner_pos = transform.voxel2scanner * voxel_pos;
              ValueType val = image.value();

              ValueType xc = scanner_pos[0] - centre[0];
              ValueType yc = scanner_pos[1] - centre[1];
              ValueType zc = scanner_pos[2] - centre[2];

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

        template <class Im1ImageType,
                  class Im2ImageType,
                  class TransformType>
          void initialise_using_image_moments (Im1ImageType& im1,
                                               Im2ImageType& im2,
                                               TransformType& transform)
          {
            CONSOLE ("initialising using image moments");
            Timer timer;
            timer.start();
            default_type  m000, m100, m010, m001, mu110, mu011, mu101, mu200, mu020, mu002;
            Eigen::Vector3 im1_centre;
            get_geometric_centre(im1, im1_centre);
            get_moments (im1, im1_centre, m000, m100, m010, m001, mu110, mu011, mu101, mu200, mu020, mu002);
            Eigen::Matrix<default_type, 3, 3> im1_covariance_matrix;
            Eigen::Matrix<default_type, 3, 1> im1_centre_of_mass;
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
            Eigen::Matrix<default_type, Eigen::Dynamic, Eigen::Dynamic> im1_evec;
            Eigen::Matrix<default_type, Eigen::Dynamic, 1> im1_eval;

            Eigen::Vector3 im2_centre;
            get_geometric_centre(im2, im2_centre);
            get_moments (im2, im2_centre, m000, m100, m010, m001, mu110, mu011, mu101, mu200, mu020, mu002);
            Eigen::Matrix<default_type, 3, 3> im2_covariance_matrix;
            Eigen::Matrix<default_type, 3, 1> im2_centre_of_mass;
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
            Eigen::Matrix<default_type, Eigen::Dynamic, Eigen::Dynamic> im2_evec;
            Eigen::Matrix<default_type, Eigen::Dynamic, 1> im2_eval;
            if (!get_sorted_eigen_vecs_vals (im2_covariance_matrix, im2_evec, im2_eval) or
                  !get_sorted_eigen_vecs_vals (im1_covariance_matrix, im1_evec, im1_eval)) {
              WARN("Image moments not successful.  Using centre of mass.");
              Eigen::Vector3 centre = (im1_centre_of_mass + im2_centre_of_mass) / 2.0;
              transform.set_centre (centre);
              Eigen::Vector3 translation = im1_centre_of_mass - im2_centre_of_mass;
              transform.set_translation (translation);
              return;
            }

            VAR(im1_eval.transpose());
            VAR(im2_eval.transpose());

            // TODO decide whether rotation is sensible using covariance eigenvalues
            // Maaref, A., & Aïssa, S. (2007). Eigenvalue distributions of wishart-type random matrices with application to the performance analysis of MIMO MRC systems. IEEE Transactions on Wireless Communications, 6(7), 2678–2689. doi:10.1109/TWC.2007.05990
            // Edelman, A. (1991). The distribution and moments of the smallest eigenvalue of a random matrix of wishart type. Linear Algebra and Its Applications, 159, 55–80. doi:10.1016/0024-3795(91)90076-9

            // std::cerr << im2_covariance_matrix.format(python_fmt) << std::endl;
            // VAR(im2_evec.format(python_fmt));
            assert(im1_evec.col(0).transpose() * im1_evec.col(1) < 0.0001);
            assert(im1_evec.col(1).transpose() * im1_evec.col(2) < 0.0001);
            assert(im1_evec.col(0).transpose() * im1_evec.col(2) < 0.0001);
            assert(im2_evec.col(0).transpose() * im2_evec.col(1) < 0.0001);
            assert(im2_evec.col(1).transpose() * im2_evec.col(2) < 0.0001);
            assert(im2_evec.col(0).transpose() * im2_evec.col(2) < 0.0001);

            // permute eigenvectors to find maximum sum of dot products between eigenvectors of image 1 and image 2
            Eigen::PermutationMatrix<Eigen::Dynamic, Eigen::Dynamic> perm(3);
            perm.setIdentity();
            Eigen::PermutationMatrix<Eigen::Dynamic, Eigen::Dynamic> bestperm(3);
            bestperm.setIdentity();
            default_type bestdotprod = (im2_evec.array() * im1_evec.array()).colwise().sum().abs().sum();
            while (std::next_permutation(perm.indices().data(), perm.indices().data()+perm.indices().size())) {
              auto im2_evec_perm = im2_evec * perm; // permute columns
              default_type dotprod = (im2_evec_perm.array() * im1_evec.array()).colwise().sum().abs().sum();
              if (dotprod > bestdotprod) bestperm = perm;
            }
            im2_evec = im2_evec * bestperm;
            im2_eval = im2_eval * bestperm;
            assert(im2_evec.col(0).transpose() * im2_evec.col(1) < 0.0001);
            assert(im2_evec.col(1).transpose() * im2_evec.col(2) < 0.0001);
            assert(im2_evec.col(0).transpose() * im2_evec.col(2) < 0.0001);


            // flip vectors to same handedness
            im2_evec.array().rowwise() *= (im2_evec.array() * im1_evec.array()).colwise().sum().cwiseQuotient((im2_evec.array() * im1_evec.array()).colwise().sum().abs()).eval();
            assert(im2_evec.col(0).transpose() * im2_evec.col(1) < 0.0001);
            assert(im2_evec.col(1).transpose() * im2_evec.col(2) < 0.0001);
            assert(im2_evec.col(0).transpose() * im2_evec.col(2) < 0.0001);

            // MAT(im1_evec);
            // MAT(im2_evec);
            // get rotation matrix
            auto im1_evec_transpose = im1_evec.transpose().eval();
            auto im2_evec_transpose = im2_evec.transpose().eval();

            Eigen::FullPivHouseholderQR<decltype(im2_evec_transpose)> dec(im2_evec_transpose);
            Eigen::Matrix<default_type, Eigen::Dynamic, Eigen::Dynamic> A (3,3);
            A = dec.solve(im1_evec_transpose);
            assert((A * im1_evec).isApprox(im2_evec));
            assert(std::abs(A.determinant() - 1.0) < 0.0001);
            A = A.transpose().eval();

            // TODO decide whether rotation matrix is sensible
            // Eigen::EigenSolver<Eigen::Matrix<default_type, 3, 3>> es((Eigen::Matrix<default_type, 3, 3>) A);
            // MAT(es.eigenvalues().real().transpose());
            // const Eigen::Matrix<default_type, 3, 3> evec (es.eigenvectors().real().cast<default_type>());
            // MAT(evec);
            // Eigen::Quaternion<default_type, Eigen::AutoAlign> quaternion (evec);
            // std::cerr << "\nw:" << quaternion.w() << " x:" << quaternion.x() << " y:" << quaternion.y() << " z:" << quaternion.z() << "\n";
            // Eigen::Quaternion<default_type, Eigen::AutoAlign> quaternion ((Eigen::Matrix<default_type, 3, 3>) A);
            // std::cerr << "\nw:" << quaternion.w() << " x:" << quaternion.x() << " y:" << quaternion.y() << " z:" << quaternion.z() << "\n";
            transform.set_matrix (A);
            Eigen::Vector3 centre = (im1_centre_of_mass + im2_centre_of_mass) / 2.0;
            transform.set_centre (centre);
            Eigen::Vector3 translation = im1_centre_of_mass - im2_centre_of_mass;
            transform.set_translation (translation);
            // transform.set_matrix (A);
            MAT(A);
            MAT(centre.transpose());
            MAT(translation.transpose());
            MAT(transform.get_transform().matrix());
            // VAR(timer.elapsed());
        }

        template <class Im1ImageType,
                  class Im2ImageType,
                  class TransformType>
          void initialise_using_image_mass (Im1ImageType& im1,
                                            Im2ImageType& im2,
                                            TransformType& transform)
          {
            CONSOLE ("initialising centre of rotation and translation using centre of mass");
            Eigen::Matrix<typename TransformType::ParameterType, 3, 1>  im1_centre_of_mass (3);
            im1_centre_of_mass.setZero();
            default_type im1_mass = 0.0;
            MR::Transform im1_transform (im1);
            // only use the first volume of a 4D file. This is important for FOD images.
            for (auto i = Loop (0, 3)(im1); i; ++i) {
              Eigen::Vector3 voxel_pos ((default_type)im1.index(0), (default_type)im1.index(1), (default_type)im1.index(2));
              Eigen::Vector3 im1_scanner = im1_transform.voxel2scanner * voxel_pos;
              im1_mass += im1.value();
              im1_centre_of_mass += im1_scanner * im1.value();
            }
            im1_centre_of_mass /= im1_mass;

            Eigen::Matrix<typename TransformType::ParameterType, 3, 1> im2_centre_of_mass (3);
            im2_centre_of_mass.setZero();
            default_type im2_mass = 0.0;
            MR::Transform im2_transform (im2);
            for (auto i = Loop(0, 3)(im2); i; ++i) {
              Eigen::Vector3 voxel_pos ((default_type)im2.index(0), (default_type)im2.index(1), (default_type)im2.index(2));
              Eigen::Vector3 im2_scanner = im2_transform.voxel2scanner * voxel_pos;
              im2_mass += im2.value();
              im2_centre_of_mass += im2_scanner * im2.value();
            }
            im2_centre_of_mass /= im2_mass;

            Eigen::Vector3 centre = (im1_centre_of_mass + im2_centre_of_mass) / 2.0;
            Eigen::Vector3 translation = im1_centre_of_mass - im2_centre_of_mass;
            // VEC(im1_centre_of_mass);
            // VEC(im2_centre_of_mass);
            // VEC(translation);
            transform.set_centre (centre);
            transform.set_translation (translation);
        }
      }
    }
  }
}
#endif
