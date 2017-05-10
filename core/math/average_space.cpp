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
      double matrix_average (vector<Eigen::MatrixXd> const &mat_in, Eigen::MatrixXd& mat_avg, bool verbose) {
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
        for (size_t i = 0; i < 10000; ++i) {
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
        return mat_s.squaredNorm();
      }
   }
 }

 namespace MR
 {

  FORCE_INLINE Eigen::Matrix<default_type, 8, 4> get_cuboid_corners (const Eigen::Matrix<default_type, 4, 1>& xyz1) {
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
    corners *= xyz1.asDiagonal();
    return corners;
  }

  FORCE_INLINE Eigen::Matrix<default_type, 8, 4> get_bounding_box (const Header& header, const Eigen::Transform<default_type, 3, Eigen::Projective>& voxel2scanner) {
    assert (header.ndim() >= 3 && "get_bounding_box: image dimension has to be >= 3");
    // width in voxels
    Eigen::Matrix<default_type, 4, 1> width = Eigen::Matrix<default_type, 4, 1>::Ones(4);
    for (size_t i = 0; i < 3; i++)
      width[i] = header.size(i) - 1.0;

    // image boundary box corners in scanner space
    Eigen::Matrix<default_type, 8, 4> corners = get_cuboid_corners (width);
    for (int i = 0; i < corners.rows(); i++)
      corners.transpose().col(i) = voxel2scanner * corners.transpose().col(i);
    return corners;
  }

  Eigen::PermutationMatrix<Eigen::Dynamic,Eigen::Dynamic> iterative_closest_point_match (
    const Eigen::MatrixXd &target_vertices, const Eigen::MatrixXd &moving_vertices) {
    assert(target_vertices.rows() == moving_vertices.rows());
    const int n = moving_vertices.rows();
    assert (n > 1 && "more than one vertex required");
    assert(target_vertices.cols() == moving_vertices.cols());

    Eigen::PermutationMatrix<Eigen::Dynamic,Eigen::Dynamic> perm(n);
    perm.setIdentity();
    for (int trow = 0; trow < n; trow++) {
      double sqnorm = std::numeric_limits<double>::max();
      int *idx = perm.indices().size() + perm.indices().data() + 1;
      for (auto mrow = perm.indices().data()+trow ; mrow < perm.indices().data() + perm.indices().size(); mrow++) {
        double sn = (target_vertices.row(trow) - moving_vertices.row(*mrow)).squaredNorm();
        if (sn < sqnorm) {
          sqnorm = sn;
          idx = mrow;
        }
      }
      assert (idx <= perm.indices().size() + perm.indices().data());
      std::iter_swap (perm.indices().data() + trow, idx);
    }
    return perm;
    // A_perm = A * perm; // permute columns
    // A_perm = perm * A; // permute rows
  }

  Eigen::Quaterniond rot_match_coordinates (const Eigen::MatrixXd &target_vertices, const Eigen::MatrixXd &moving_vertices) {
    // rotate moving_vertices to minimise distance between closest vertices between target_vertices and moving_vertices
    const int nt = target_vertices.rows();
    const int nm = moving_vertices.rows();
    assert (nm > 1 && "more than one vertex required");
    assert(target_vertices.cols() == moving_vertices.cols());
    assert(target_vertices.cols() == 3 && "3D");

    double sqnorm = std::numeric_limits<double>::max();
    int tidx = 0, midx = 0;
    for (int mrow = 0; mrow < nm; mrow++) {
      for (int trow = 0; trow < nt; trow++) {
        double sn = (target_vertices.row(trow) - moving_vertices.row(mrow)).squaredNorm();
        if (sn < sqnorm) {
          sqnorm = sn;
          tidx = trow;
          midx = mrow;
        }
      }
      break;
    }
    Eigen::Vector3d tvec = target_vertices.row(tidx).transpose();
    Eigen::Vector3d mvec = moving_vertices.row(midx).transpose();

    // quat1: rotate line of direction of moving_vertices to the line of target_vertices
    Eigen::Quaterniond quat1 = Eigen::Quaterniond().setFromTwoVectors(mvec, tvec);
    const Eigen::Matrix3d R (quat1);

    midx = midx + 1 % nm;
    const Eigen::Vector3d mvec_rot = moving_vertices.row(midx) * R.transpose();
    sqnorm = std::numeric_limits<double>::max();
    for (int trow = 0; trow < nt; trow++) {
      if (trow == tidx)
        continue;
      double sn = (target_vertices.row(trow) - mvec_rot.transpose()).squaredNorm();
      if (sn < sqnorm) {
        sqnorm = sn;
        tidx = trow;
      }
    }
    auto quat2 = Eigen::Quaterniond::FromTwoVectors ( mvec_rot.transpose(), target_vertices.row(tidx).transpose());
    Eigen::Quaterniond res (quat2 * quat1);

    return res;
  }


  transform_type align_corresponding_vertices (const Eigen::MatrixXd &target_vertices, const Eigen::MatrixXd &moving_vertices, bool scale) {
      //  this function aligns two sets of vertices which must have corresponding vertices stored in the same row
      //
      //  scale == false --> Kabsch: "A solution for the best rotation to relate two sets of vectors"
      //  minimise (target_vertices.row(i) - M * moving_vertices.row(i) + t).squaredNorm();
      //
      //  scale == true --> Umeyama: "Least-Squares Rigid Motion Using SVD"
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
      Eigen::MatrixXd cov = (target_centered.adjoint() * moving_centered) / default_type (n - 1);

      Eigen::JacobiSVD<Eigen::Matrix3d> svd (cov, Eigen::ComputeFullU | Eigen::ComputeFullV);

      // rotation matrix
      Eigen::MatrixXd R = svd.matrixV() * svd.matrixU().transpose();

      // calculate determinant of V*U^T to disambiguate rotation sign
      default_type f_det = R.determinant();
      Eigen::Vector3d e(1, 1, (f_det < 0)? -1 : 1);

      // recompute the rotation if the determinant was negative
      if (f_det < 0)
        R.noalias() = svd.matrixV() * e.asDiagonal() * svd.matrixU().transpose();

      // renormalize the rotation
      // R = Eigen::Quaterniond(R).normalized().toRotationMatrix();

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

  void compute_average_voxel2scanner (Eigen::Transform<default_type, 3, Eigen::Projective>& average_v2s_trafo,
                                      Eigen::Vector3d& average_space_extent,
                                      Eigen::Vector3d& projected_voxel_sizes,
                                      const vector<Header>& input_headers,
                                      const Eigen::Matrix<default_type, 4, 1>& padding,
                                      const vector<Eigen::Transform<default_type, 3, Eigen::Projective>>& transform_header_with,
                                      int voxel_subsampling) {
    const size_t num_images = input_headers.size();
    DEBUG ("compute_average_voxel2scanner num_images:" + str(num_images));
    vector<Eigen::Transform<default_type, 3, Eigen::Projective>> transformation_matrices;
    Eigen::MatrixXd bounding_box_corners = Eigen::MatrixXd::Zero (8 * num_images, 4);

    for (size_t iFile = 0; iFile < num_images; iFile++) {
      Eigen::Transform< default_type, 3, Eigen::Projective> v2s_trafo = (Eigen::Transform< default_type, 3, Eigen::Projective>) Transform(input_headers[iFile]).voxel2scanner;

      if (transform_header_with.size()) {
        assert (transform_header_with.size() == input_headers.size());
        v2s_trafo = transform_header_with[iFile] * v2s_trafo;
      }
      transformation_matrices.push_back(v2s_trafo);

      bounding_box_corners.template block<8,4>(iFile*8,0) = get_bounding_box (input_headers[iFile], v2s_trafo);
      // voxel_diagonals.col(iFile) = v2s_trafo * voxel_diagonals.col(iFile);
    }

    // Get average rotation of coordinate system
    // rotation is smallest rotation possible to rotate image towards scanner coordinate system grid
    // to factor out large rotations. For this, the matrix Frechet mean (average_matrix) is not suitable.
    // average quaternion calculation from http://www.acsu.buffalo.edu/~johnc/ave_quat07.pdf
    Eigen::MatrixXd ScannerSpaceAxis3 = Eigen::MatrixXd::Identity(3,3);
    Eigen::Matrix<default_type, 6, 3> ScannerSpaceAxis6;
    ScannerSpaceAxis6.block<3,3>(0,0) = ScannerSpaceAxis3;
    ScannerSpaceAxis6.block<3,3>(3,0) = -ScannerSpaceAxis3;

    Eigen::MatrixXd quaternions = Eigen::MatrixXd(4, num_images);
    for (size_t itrafo = 0; itrafo < num_images; itrafo++) {
      Eigen::MatrixXd Other = ScannerSpaceAxis3 * transformation_matrices[itrafo].rotation().transpose();
      Eigen::Quaterniond quat = rot_match_coordinates (ScannerSpaceAxis6, Other);
      quaternions.col(itrafo) = quat.coeffs();  // internally the coefficients are stored in the following order: [x, y, z, w]
    }
    Eigen::SelfAdjointEigenSolver<Eigen::MatrixXd> es (quaternions * quaternions.transpose(), Eigen::ComputeEigenvectors);
    Eigen::Quaterniond average_quat;
    average_quat.coeffs() = es.eigenvectors().col(3).transpose();
    const Eigen::Matrix3d Raverage (average_quat.toRotationMatrix());
    Eigen::MatrixXd temp = Eigen::MatrixXd (3, num_images);
    for (size_t itrafo = 0; itrafo < num_images; itrafo++) {
      temp.col(itrafo) = (Raverage * transformation_matrices[itrafo].linear()).cwiseAbs().rowwise().sum();
    }

    switch (voxel_subsampling) {
      case 0: // maximum voxel size determined by minimum of all projected input image voxel sizes
          projected_voxel_sizes = temp.transpose().colwise().minCoeff().transpose();
        break;
      case 1: // maximum voxel size determined by mean of all projected input image voxel sizes
          projected_voxel_sizes = temp.transpose().colwise().mean().transpose();
          break;
      default:
        assert( 0 && "compute_average_voxel2scanner: invalid voxel_subsampling option");
        break;
      }

    Eigen::MatrixXd mat_avg = Eigen::MatrixXd::Zero (4, 4);
    mat_avg.block<3,3>(0,0) = Raverage.transpose();
    mat_avg.col(0) *= projected_voxel_sizes(0);
    mat_avg.col(1) *= projected_voxel_sizes(1);
    mat_avg.col(2) *= projected_voxel_sizes(2);
    mat_avg(3,3) = 1;

    average_v2s_trafo.matrix() = mat_avg; // average_v2s_trafo has zero translation
    Eigen::Transform<default_type, 3, Eigen::Projective> average_s2v_trafo = average_v2s_trafo.inverse (Eigen::Projective);

    // set extent in voxels and translation of average_v2s_trafo
    {
      // transform all image corners into inverse average space
      Eigen::MatrixXd bounding_box_corners_inv = bounding_box_corners * average_s2v_trafo.matrix().transpose();
      Eigen::VectorXd bounding_box_corners_inv_min = bounding_box_corners_inv.colwise().minCoeff();
      Eigen::VectorXd bounding_box_corners_inv_max = bounding_box_corners_inv.colwise().maxCoeff();
      Eigen::VectorXd bounding_box_corners_inv_extent = (bounding_box_corners_inv_max - bounding_box_corners_inv_min).cwiseAbs();
      bounding_box_corners_inv_extent += 2.0 * padding;
      bounding_box_corners_inv_extent(3) = 1.0;

      average_space_extent = bounding_box_corners_inv_extent.head<3>();
      average_space_extent(0) = std::round(average_space_extent(0)) + 1;
      average_space_extent(1) = std::round(average_space_extent(1)) + 1;
      average_space_extent(2) = std::round(average_space_extent(2)) + 1;

      Eigen::Vector3d c000 = average_v2s_trafo.linear() * (bounding_box_corners_inv_min - padding).head<3>(); // voxel space min corner
      average_v2s_trafo.matrix().col(3).template head<3>() = c000.template head<3>();
    }
  }

  Header compute_minimum_average_header (const vector<Header>& input_headers,
                                         int voxel_subsampling,
                                         const Eigen::Matrix<default_type, 4, 1>& padding,
                                         const vector<Eigen::Transform<default_type, 3, Eigen::Projective>>& transform_header_with) {
    Eigen::Transform<default_type, 3, Eigen::Projective> average_v2s_trafo;
    Eigen::Vector3d average_space_voxel_extent;
    Eigen::Vector3d projected_voxel_sizes;
    compute_average_voxel2scanner (average_v2s_trafo, average_space_voxel_extent, projected_voxel_sizes, input_headers, padding, transform_header_with, voxel_subsampling);

    // create average space header
    Header header_out;
    header_out.ndim() = 3;
    for (size_t i = 0; i < 3; i++)
      header_out.spacing(i) = projected_voxel_sizes(i);
    DEBUG("compute_minimum_average_header header_out.spacing: "+str(header_out.spacing(0))+", "+str(header_out.spacing(1))+", "+str(header_out.spacing(2)));

    header_out.transform().linear() = average_v2s_trafo.rotation();
    header_out.transform().translation() = average_v2s_trafo.translation();

    for (size_t i = 0; i < 3; i++) {
      header_out.size(i) = std::ceil (average_space_voxel_extent(i));
      if (header_out.size(i) < 1)
        throw Exception ("average space header has zero voxels in dimension " + str(i) + ". Increase resolution?");
    }
    DEBUG("compute_minimum_average_header header_out.size: "+str(header_out.size(0))+", "+str(header_out.size(1))+", "+str(header_out.size(2)));

    return header_out;
  }
}
