/* Copyright (c) 2008-2017 the MRtrix3 contributors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, you can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * MRtrix is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * For more details, see http://www.mrtrix.org/.
 */



#include "math/average_space.h"


namespace MR
{
   namespace Math
   {
      void matrix_average (vector<Eigen::MatrixXd> const &mat_in, Eigen::MatrixXd& mat_avg, bool verbose) {
        const size_t rows = mat_in[0].rows();
        const size_t cols = mat_in[0].cols();
        const size_t N = mat_in.size();
        assert(rows);
        assert(cols);
        // check input
        for (const auto &mat: mat_in) {
          if (cols != (size_t) mat.cols() or rows != (size_t) mat.rows())
            throw Exception("matrix average cannot be computed for matrices of different size");
        }
        mat_avg.resize(rows, cols);
        mat_avg.setIdentity();

        Eigen::MatrixXd mat_s (rows, cols);  // sum
        Eigen::MatrixXd mat_l (rows, cols);
        for (size_t i = 0; i<1000; ++i) {
          mat_s.setZero(rows,cols);
          Eigen::ColPivHouseholderQR<Eigen::MatrixXd> dec (mat_avg);
          for (size_t j = 0; j < N; ++j) {
            mat_l = dec.solve (mat_in[j]); // solve mat_avg * mat_l = mat_in[j] for mat_l
            // std::cout << "mat_avg*mat_l - mat_in[j]:\n" << mat_avg*mat_l - mat_in[j] <<std::endl;
            mat_s += mat_l.log().real();
          }
          mat_s *= 1./N;
          mat_avg *= mat_s.exp();
          if (verbose) std::cerr << i << " mat_s.squaredNorm(): " << mat_s.squaredNorm() << std::endl;
          if (mat_s.squaredNorm() < 1E-20) {
            break;
          }
        }
      }
   }
 }

 namespace MR
 {

  Eigen::Matrix<default_type, 8, 4> get_cuboid_corners (const Eigen::Matrix<default_type, 4, 1>& xzx1) {

    Eigen::Matrix<default_type, 8, 4>  corners;
    // MatrixType faces = MatrixType(8,4);
    // faces << 1, 2, 3, 4,   2, 6, 7, 3,   4, 3, 7, 8,   1, 5, 8, 4,   1, 2, 6, 5,   5, 6, 7, 8;
    corners << 0.0, 0.0, 0.0, 1.0,
               0.0, 1.0, 0.0, 1.0,
               1.0, 1.0, 0.0, 1.0,
               1.0, 0.0, 0.0, 1.0,
               0.0, 0.0, 1.0, 1.0,
               0.0, 1.0, 1.0, 1.0,
               1.0, 1.0, 1.0, 1.0,
               1.0, 0.0, 1.0, 1.0;
    corners *= xzx1.asDiagonal();
    return corners;
  }

  Eigen::Matrix<default_type, 8, 4> get_bounding_box (const Header& header, const Eigen::Transform<default_type, 3, Eigen::Projective>& voxel2scanner)
  {
    assert (header.ndim() >= 3 && "get_bounding_box: image dimension has to be >= 3");
    Eigen::Matrix<default_type, 4, 1> width = Eigen::Matrix<default_type, 4, 1>::Ones(4);
    // width in voxels
    for (size_t i = 0; i < 3; i++) {
      width(i) = header.size(i) - 1.0;
    }
    // get image boundary box corners in scanner space
    Eigen::Matrix<default_type, 8, 4> corners = get_cuboid_corners(width);
    for (int i = 0; i < corners.rows(); i++) {
      corners.transpose().col(i) = (voxel2scanner * corners.transpose().col(i));
    }
    return corners;
  }

  void compute_average_voxel2scanner (Eigen::Transform<default_type, 3, Eigen::Projective>& average_v2s_trafo,
                                      Eigen::VectorXd& average_space_extent,
                                      Eigen::MatrixXd& projected_voxel_sizes,
                                      const vector<Header>& input_headers,
                                      const Eigen::Matrix<default_type, 4, 1>& padding,
                                      const vector<Eigen::Transform<default_type, 3, Eigen::Projective>>& transform_header_with) {
    const size_t num_images = input_headers.size();
    vector<Eigen::MatrixXd> transformation_matrices;
    Eigen::MatrixXd bounding_box_corners = Eigen::MatrixXd::Zero (8 * num_images, 4);
    Eigen::MatrixXd voxel_diagonals = Eigen::MatrixXd::Ones(4, num_images);
    voxel_diagonals.row(3) = Eigen::VectorXd::Zero(num_images);

    for (size_t iFile = 0; iFile < num_images; iFile++) {
      auto v2s_trafo = (Eigen::Transform< default_type, 3, Eigen::Projective>) Transform(input_headers[iFile]).voxel2scanner;

      if (transform_header_with.size() > iFile)
        v2s_trafo = transform_header_with[iFile] * v2s_trafo;
      transformation_matrices.push_back(v2s_trafo.matrix());

      bounding_box_corners.template block<8,4>(iFile*8,0) = get_bounding_box(input_headers[iFile], v2s_trafo);
      voxel_diagonals.col(iFile) = v2s_trafo * voxel_diagonals.col(iFile);
    }

    Eigen::MatrixXd mat_avg = Eigen::MatrixXd(4, 4);
    Math::matrix_average (transformation_matrices, mat_avg);

    average_v2s_trafo.matrix() = mat_avg;

    Eigen::Transform<default_type, 3, Eigen::Projective> average_s2v_trafo = average_v2s_trafo.inverse(Eigen::Projective);

    // transform all image corners into inverse average space
    Eigen::MatrixXd bounding_box_corners_inv = bounding_box_corners;
    for (int i = 0; i < bounding_box_corners_inv.rows(); i++)
      bounding_box_corners_inv.transpose().col(i) = (average_s2v_trafo * bounding_box_corners.transpose().col(i));
    // minimum axis-aligned corners in inverse average space
    Eigen::VectorXd bounding_box_corners_inv_min = bounding_box_corners_inv.colwise().minCoeff();
    Eigen::VectorXd bounding_box_corners_inv_max = bounding_box_corners_inv.colwise().maxCoeff();
    Eigen::VectorXd bounding_box_corners_inv_width = bounding_box_corners_inv_max - bounding_box_corners_inv_min;
    bounding_box_corners_inv_width += 2.0 * padding;
    bounding_box_corners_inv_width(3) = 1.0;

    bounding_box_corners = get_cuboid_corners(bounding_box_corners_inv_width);

    // transform boundary box corners back into scanner space
    for (int i = 0; i < bounding_box_corners.rows(); i++) {
      bounding_box_corners.row(i) += bounding_box_corners_inv_min - padding;
      bounding_box_corners.row(i)(3) = 1.0;
      bounding_box_corners.row(i) = (average_v2s_trafo * bounding_box_corners.row(i).transpose()).transpose();
    }

    // set translation to first corner (0, 0, 0, 1)
    average_v2s_trafo.matrix().col(3).template head<3>() = bounding_box_corners.row(0).template head<3>();

    // define extent via bounding box diagonal:
    average_space_extent.resize(4);
    average_space_extent = bounding_box_corners.row(6) - bounding_box_corners.row(0);
    average_space_extent(3) = 1.0;

    // project voxel diagonals onto average space coordinate axes
    Eigen::MatrixXd average_xyz = Eigen::MatrixXd::Zero(4,3);
    average_xyz.template block<3,3>(0,0) = average_v2s_trafo.linear() * Eigen::DiagonalMatrix<default_type,3,3> (1.0, 1.0, 1.0);
    for (size_t dim = 0; dim < 3; dim++) {
      average_xyz.col(dim) /= average_xyz.col(dim).norm();
    }
    projected_voxel_sizes.noalias() = (average_xyz.transpose() * voxel_diagonals).cwiseAbs();
  }

  Header compute_minimum_average_header (const vector<Header>& input_headers,
                                         int voxel_subsampling,
                                         const Eigen::Matrix<default_type, 4, 1>& padding,
                                         const vector<Eigen::Transform<default_type, 3, Eigen::Projective>>& transform_header_with) {
    Eigen::Transform<default_type, 3, Eigen::Projective> average_v2s_trafo;
    Eigen::VectorXd average_space_extent;
    Eigen::MatrixXd projected_voxel_sizes;
    compute_average_voxel2scanner (average_v2s_trafo, average_space_extent, projected_voxel_sizes, input_headers, padding, transform_header_with);
    assert(average_space_extent.isApprox(average_space_extent.cwiseAbs()));

    const Eigen::MatrixXd R = average_v2s_trafo.rotation();
    const Eigen::MatrixXd Scale = R.transpose() * average_v2s_trafo.linear(); // average_v2s_trafo == R * S
    const Eigen::VectorXd scale = Eigen::Matrix<default_type, 4, 1> (Scale(0,0), Scale(1,1), Scale(2,2), 1.0);
    Eigen::Vector3d spacing;
    // Eigen::Matrix<default_type, 4, 1> xyz1;

    switch (voxel_subsampling) {
      case 0: // maximum voxel size determined by minimum of all projected input image voxel sizes
          spacing << projected_voxel_sizes.transpose().colwise().minCoeff().transpose();
        break;

      case 1: // maximum voxel size determined by mean of all projected input image voxel sizes
          spacing << projected_voxel_sizes.transpose().colwise().mean().transpose();
          break;

      default:
        assert( 0 && "compute_minimum_average_header: invalid voxel_subsampling option");
        break;
    }

    // create average space header
    Header header_out;
    header_out.ndim() = 3;
    for (size_t i = 0; i < 3; i++)
      header_out.spacing(i) = spacing(i);

    header_out.transform().linear() = average_v2s_trafo.rotation();
    header_out.transform().translation() = average_v2s_trafo.translation();

    {
      Eigen::VectorXd size = (R.transpose() * average_space_extent.head<3>()).cwiseQuotient(spacing);
      for (size_t i = 0; i < 3; i++)
        header_out.size(i) = std::ceil (size(i));
    }

    return header_out;
  }
}
