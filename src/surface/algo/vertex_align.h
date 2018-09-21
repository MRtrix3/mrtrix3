/*
 * Copyright (c) 2008-2018 the MRtrix3 contributors.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, you can obtain one at http://mozilla.org/MPL/2.0/
 *
 * MRtrix3 is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * For more details, see http://www.mrtrix.org/
 */


#ifndef __surface_algo_meshalign_h__
#define __surface_algo_meshalign_h__

#include <Eigen/Geometry>
#include <Eigen/Dense>

#include "image.h"
#include "types.h"
#include "surface/mesh.h"



namespace MR
{
  namespace Surface
  {
    namespace Algo
    {

      Eigen::MatrixXd vert2mat (const VertexList& vertices)
      {
        Eigen::MatrixXd out (vertices.size(), 3);
        size_t k = 0;
        for (Vertex vertex : vertices) {
          out.row(k++) = vertex;
        }
        return out;
      }


      double find_closest_vertices (const Eigen::MatrixXd& target, const Eigen::MatrixXd& source, Eigen::MatrixXd& out)
      {
        Eigen::VectorXd dif (target.rows());
        Eigen::Vector3 ref, shift = target.colwise().mean() - source.colwise().mean();
        double dist = 0.0; size_t idx;
        for (size_t k = 0; k < target.rows(); k++) {
          ref = target.row(k).transpose() - shift;
          dif = (source.rowwise() - ref.transpose()).rowwise().squaredNorm();
          dist += dif.minCoeff(&idx);
          out.row(k) = source.row(idx);
        }
        return dist;
      }


      transform_type align_corresponding_vertices (const Eigen::MatrixXd &target_vertices, const Eigen::MatrixXd &moving_vertices, bool scale) {
        //  this function aligns two sets of vertices which must have corresponding vertices stored in the same row
        //
        //  scale == false --> Kabsch
        //  minimise (target_vertices.row(i) - M * moving_vertices.row(i) + t).squaredNorm();
        //
        //  scale == true --> Umeyama
        //  nonrigid version of Kabsch algorithm that also includes scale (not shear)
        //
        assert(target_vertices.rows() == moving_vertices.rows());
        const size_t n = moving_vertices.rows();
        assert (n > 2);
  
        assert(target_vertices.cols() == moving_vertices.cols());
        assert(target_vertices.cols() == 3 && "align_corresponding_vertices implemented only for 3D data");
  
        Eigen::VectorXd moving_centre = moving_vertices.colwise().mean();
        Eigen::VectorXd target_centre = target_vertices.colwise().mean();
        Eigen::MatrixXd moving_centered = moving_vertices.rowwise() - moving_centre.transpose();
        Eigen::MatrixXd target_centered = target_vertices.rowwise() - target_centre.transpose();
        Eigen::MatrixXd cov = (moving_centered.adjoint() * target_centered);
  
        Eigen::JacobiSVD<Eigen::Matrix3d> svd (cov, Eigen::ComputeFullU | Eigen::ComputeFullV);
  
        // rotation matrix
        Eigen::Matrix3d R = svd.matrixV() * svd.matrixU().transpose();
  
        // calculate determinant of V*U^T to disambiguate rotation sign
        default_type f_det = R.determinant();
        Eigen::Vector3d e (1, 1, (f_det < 0)? -1 : 1);
  
        // recompute the rotation if the determinant was negative
        if (f_det < 0)
          R.noalias() = svd.matrixV() * e.asDiagonal() * svd.matrixU().transpose();
  
        // renormalize the rotation, R needs to be Matrix3d
        R = Eigen::Quaterniond(R).normalized().toRotationMatrix();
  
        if (scale) {
          default_type fsq = 0;
          for (size_t i = 0; i < n; ++ i)
            fsq += moving_centered.row(i).squaredNorm();
          // calculate and apply the scale
          default_type fscale = svd.singularValues().dot(e) / fsq;
          R *= fscale;
        }
  
        transform_type T;
        T.linear() = R;
        T.translation() = target_centre - (R * moving_centre);
        return T;
      }


      transform_type iterative_closest_point (const Eigen::MatrixXd& target, const Eigen::MatrixXd& source, bool scale, double& dist)
      {
        transform_type T;
        Eigen::MatrixXd target_t = target;
        Eigen::MatrixXd source_map (target.rows(), 3);
        double prevdist = std::numeric_limits<double>::infinity();
        for (int k = 0; k < 10; k++) {
          dist = find_closest_vertices(target_t, source, source_map);
          T = align_corresponding_vertices(target, source_map, scale);
          target_t.transpose() = T.inverse() * target.leftCols<3>().transpose();
          if (std::fabs(dist - prevdist) < 1e-3) break;
          prevdist = dist;
        }
        return T;
      }

      transform_type iterative_closest_point (const Eigen::MatrixXd& target, const Eigen::MatrixXd& source, bool scale)
      {
        double dist;
        return iterative_closest_point(target, source, scale, dist);
      }


    }
  }
}

#endif

