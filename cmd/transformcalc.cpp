/* Copyright (c) 2008-2023 the MRtrix3 contributors.
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
#include <unsupported/Eigen/MatrixFunctions>
#include <algorithm>
#include "command.h"
#include "math/math.h"
#include "math/average_space.h"
#include "image.h"
#include "file/nifti_utils.h"
#include "transform.h"
#include "file/key_value.h"


using namespace MR;
using namespace App;

const char* operations[] = {
  "invert",
  "half",
  "rigid",
  "header",
  "average",
  "interpolate",
  "decompose",
  "align_vertices_rigid",
  "align_vertices_rigid_scale",
  NULL
};

void usage ()
{
  AUTHOR = "Max Pietsch (maximilian.pietsch@kcl.ac.uk)";

  SYNOPSIS = "Perform calculations on linear transformation matrices";

  ARGUMENTS
  + Argument ("inputs", "the input(s) for the specified operation").allow_multiple()
  + Argument ("operation", "the operation to perform, one of: " + join(operations, ", ") + " (see description section for details).").type_choice (operations)
  + Argument ("output", "the output transformation matrix.").type_file_out ();

  EXAMPLES
  + Example ("Invert a transformation",
             "transformcalc matrix_in.txt invert matrix_out.txt",
             "")

  + Example ("Calculate the matrix square root of the input transformation (halfway transformation)",
             "transformcalc matrix_in.txt half matrix_out.txt",
             "")

  + Example ("Calculate the rigid component of an affine input transformation",
             "transformcalc affine_in.txt rigid rigid_out.txt",
             "")

  + Example ("Calculate the transformation matrix from an original image and an image with modified header",
             "transformcalc mov mapmovhdr header output",
             "")

  + Example ("Calculate the average affine matrix of a set of input matrices",
             "transformcalc input1.txt ... inputN.txt average matrix_out.txt",
             "")

  + Example ("Create interpolated transformation matrix between two inputs",
             "transformcalc input1.txt input2.txt interpolate matrix_out.txt",
             "Based on matrix decomposition with linear interpolation of "
             "translation, rotation and stretch described in: "
             "Shoemake, K., Hill, M., & Duff, T. (1992). Matrix Animation and Polar Decomposition. "
             "Matrix, 92, 258-264. doi:10.1.1.56.1336")

  + Example ("Decompose transformation matrix M into translation, rotation and stretch and shear (M = T * R * S)",
             "transformcalc matrix_in.txt decompose matrixes_out.txt",
             "The output is a key-value text file containing: "
             "scaling: vector of 3 scaling factors in x, y, z direction; "
             "shear: list of shear factors for xy, xz, yz axes; "
             "angles: list of Euler angles about static x, y, z axes in radians in the range [0:pi]x[-pi:pi]x[-pi:pi]; "
             "angle_axis: angle in radians and rotation axis; "
             "translation : translation vector along x, y, z axes in mm; "
             "R: composed roation matrix (R = rot_x * rot_y * rot_z); "
             "S: composed scaling and shear matrix")

  + Example ("Calculate transformation that aligns two images based on sets of corresponding landmarks",
             "transformcalc input moving.txt fixed.txt align_vertices_rigid rigid.txt",
             "Similary, 'align_vertices_rigid_scale' produces an affine matrix (rigid and global scale). "
             "Vertex coordinates are in scanner space, corresponding vertices must be stored in the same row "
             "of moving.txt and fixed.txt. Requires 3 or more vertices in each file. "
             "Algorithm: Kabsch 'A solution for the best rotation to relate two sets of vectors' DOI:10.1107/S0567739476001873");

}

template <typename T> int sgn(T val) {
    return (T(0) < val) - (val < T(0));
}

transform_type align_corresponding_vertices (const Eigen::MatrixXd &src_vertices, const Eigen::MatrixXd &trg_vertices, bool scale) {
  //  this function aligns two sets of vertices which must have corresponding vertices stored in the same row
  //
  //  scale == false --> Kabsch
  //  minimise (src_vertices.row(i) - M * trg_vertices.row(i) + t).squaredNorm();
  //
  //  scale == true
  //  nonrigid version of Kabsch algorithm that also includes scale (not shear)
  //
  assert(src_vertices.rows() == trg_vertices.rows());
  const size_t n = trg_vertices.rows();
  if (n < 3)
    throw Exception ("vertex alignment requires at least 3 points");

  assert(src_vertices.cols() == trg_vertices.cols());
  assert(src_vertices.cols() == 3 && "align_corresponding_vertices implemented only for 3D data");

  Eigen::VectorXd trg_centre = trg_vertices.colwise().mean();
  Eigen::VectorXd src_centre = src_vertices.colwise().mean();
  Eigen::MatrixXd trg_centred = trg_vertices.rowwise() - trg_centre.transpose();
  Eigen::MatrixXd src_centred = src_vertices.rowwise() - src_centre.transpose();
  Eigen::MatrixXd cov = (src_centred.adjoint() * trg_centred) / default_type (n - 1);

  Eigen::JacobiSVD<Eigen::Matrix3d> svd (cov, Eigen::ComputeFullU | Eigen::ComputeFullV);

  // rotation matrix
  Eigen::Matrix3d R = svd.matrixV() * svd.matrixU().transpose();

  // calculate determinant of V*U^T to disambiguate rotation sign
  default_type f_det = R.determinant();
  Eigen::Vector3d e(1, 1, (f_det < 0)? -1 : 1);

  // recompute the rotation if the determinant was negative
  if (f_det < 0)
    R.noalias() = svd.matrixV() * e.asDiagonal() * svd.matrixU().transpose();

  // renormalize the rotation, R needs to be Matrix3d
  R = Eigen::Quaterniond(R).normalized().toRotationMatrix();

  if (scale) {
    default_type fsq_t = 0.0;
    default_type fsq_s = 0.0;
    for (size_t i = 0; i < n; ++ i) {
      fsq_t += trg_centred.row(i).squaredNorm();
      fsq_s += src_centred.row(i).squaredNorm();
    }
    // calculate and apply the scale
    default_type fscale = sqrt(fsq_t / fsq_s); // Umeyama: svd.singularValues().dot(e) / fsq;
    DEBUG("scaling: "+str(fscale));
    R *= fscale;
  }

  transform_type T1 = transform_type::Identity();
  transform_type T2 = transform_type::Identity();
  transform_type T3 = transform_type::Identity();
  T1.translation() = trg_centre;
  T2.linear() = R;
  T3.translation() = -src_centre;
  return T1 * T2 * T3;
}

void run ()
{
  const size_t num_inputs = argument.size() - 2;
  const int op = argument[num_inputs];
  const std::string& output_path = argument.back();

  switch (op) {
    case 0: { // invert
      if (num_inputs != 1)
        throw Exception ("invert requires 1 input");
      transform_type input = load_transform (argument[0]);
      save_transform (input.inverse(), output_path);
      break;
    }
    case 1: { // half
      if (num_inputs != 1)
        throw Exception ("half requires 1 input");
      Eigen::Transform<default_type, 3, Eigen::Projective> input = load_transform (argument[0]);
      transform_type output;
      Eigen::Matrix<default_type, 4, 4> half = input.matrix().sqrt();
      output.matrix() = half.topLeftCorner(3,4);
      save_transform (output, output_path);
      break;
    }
    case 2: { // rigid
      if (num_inputs != 1)
        throw Exception ("rigid requires 1 input");
      transform_type input = load_transform (argument[0]);
      transform_type output (input);
      output.linear() = input.rotation();
      save_transform (output, output_path);
      break;
    }
    case 3: { // header
      if (num_inputs != 2)
        throw Exception ("header requires 2 inputs");
      auto orig_header = Header::open (argument[0]);
      auto modified_header = Header::open (argument[1]);

      transform_type forward_transform = Transform(modified_header).voxel2scanner * Transform(orig_header).voxel2scanner.inverse();
      save_transform (forward_transform.inverse(), output_path);
      break;
    }
    case 4: { // average
      if (num_inputs < 2)
        throw Exception ("average requires at least 2 inputs");
      transform_type transform_out;
      Eigen::Transform<default_type, 3, Eigen::Projective> Tin;
      Eigen::MatrixXd Min;
      vector<Eigen::MatrixXd> matrices;
      for (size_t i = 0; i < num_inputs; i++) {
        DEBUG(str(argument[i]));
        Tin = load_transform (argument[i]);
        matrices.push_back(Tin.matrix());
      }

      Eigen::MatrixXd average_matrix;
      Math::matrix_average ( matrices, average_matrix);
      transform_out.matrix() = average_matrix.topLeftCorner(3,4);
      save_transform (transform_out, output_path);
      break;
    }
    case 5: { // interpolate
      if (num_inputs != 3)
        throw Exception ("interpolation requires 3 inputs");
      transform_type transform1 = load_transform (argument[0]);
      transform_type transform2 = load_transform (argument[1]);
      default_type t = parse_floats(argument[2])[0];

      transform_type transform_out;

      if (t < 0.0 || t > 1.0)
        throw Exception ("t has to be in the interval [0,1]");

      Eigen::MatrixXd M1 = transform1.linear();
      Eigen::MatrixXd M2 = transform2.linear();
      if (sgn(M1.determinant()) != sgn(M2.determinant()))
        WARN("transformation determinants have different signs");

      Eigen::Matrix3d R1 = transform1.rotation();
      Eigen::Matrix3d R2 = transform2.rotation();
      Eigen::Quaterniond Q1(R1);
      Eigen::Quaterniond Q2(R2);
      Eigen::Quaterniond Qout;

      // stretch (shear becomes roation and stretch)
      Eigen::MatrixXd S1 = R1.transpose() * M1;
      Eigen::MatrixXd S2 = R2.transpose() * M2;
      if (!M1.isApprox(R1*S1))
        WARN ("M1 matrix decomposition might have failed");
      if (!M2.isApprox(R2*S2))
        WARN ("M2 matrix decomposition might have failed");

      transform_out.translation() = ((1.0 - t) * transform1.translation() + t * transform2.translation());
      Qout = Q1.slerp(t, Q2);
      transform_out.linear() = Qout * ((1 - t) * S1 + t * S2);
      INFO("\n"+str(transform_out.matrix().format(
        Eigen::IOFormat(Eigen::FullPrecision, 0, ", ", ",\n", "[", "]", "[", "]"))));
      save_transform (transform_out, output_path);
      break;
    }
    case 6: { // decompose
      if (num_inputs != 1)
        throw Exception ("decomposition requires 1 input");
      transform_type transform = load_transform (argument[0]);

      Eigen::MatrixXd M = transform.linear();
      Eigen::Matrix3d R = transform.rotation();
      Eigen::MatrixXd S = R.transpose() * M;
      if (!M.isApprox(R*S))
        WARN ("matrix decomposition might have failed");

      Eigen::Vector3d euler_angles = R.eulerAngles(0, 1, 2);
      assert (R.isApprox((Eigen::AngleAxisd(euler_angles[0], Eigen::Vector3d::UnitX())
              * Eigen::AngleAxisd(euler_angles[1], Eigen::Vector3d::UnitY())
              * Eigen::AngleAxisd(euler_angles[2], Eigen::Vector3d::UnitZ())).matrix()));

      Eigen::RowVector4d angle_axis;
      {
        auto AA = Eigen::AngleAxis<default_type> (R);
        angle_axis(0) = AA.angle();
        angle_axis.block<1,3>(0,1) = AA.axis();
      }


      File::OFStream out (output_path);
      out << "# " << App::command_history_string << "\n";
      Eigen::IOFormat fmt(Eigen::FullPrecision, Eigen::DontAlignCols, " ", "\n", "", "", "", "\n");
      out << "scaling: "     << Eigen::RowVector3d(S(0,0), S(1,1), S(2,2)).format(fmt);
      out << "shear: "       << Eigen::RowVector3d(S(0,1), S(0,2), S(1,2)).format(fmt);
      out << "angles: "      << euler_angles.transpose().format(fmt);
      out << "angle_axis: "   << angle_axis.format(fmt);
      out << "translation: " << transform.translation().transpose().format(fmt);
      out << "R: " << R.row(0).format(fmt);
      out << "R: " << R.row(1).format(fmt);
      out << "R: " << R.row(2).format(fmt);
      out << "S: " << S.row(0).format(fmt);
      out << "S: " << S.row(1).format(fmt);
      out << "S: " << S.row(2).format(fmt);
      out << "jacobian_det: " << str(transform.linear().topLeftCorner<3,3>().determinant()) << "\n";

      break;
    }
    case 7: case 8: { // align_vertices_rigid and align_vertices_rigid_scale
      if (num_inputs != 2)
        throw Exception ("align_vertices_rigid requires 2 inputs");
      const Eigen::MatrixXd target_vertices = load_matrix (argument[0]);
      const Eigen::MatrixXd moving_vertices = load_matrix (argument[1]);
      const transform_type T = align_corresponding_vertices (moving_vertices, target_vertices, op==8);
      save_transform (T, output_path);
      break;
    }
    default: assert (0);
  }


}
