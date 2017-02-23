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
#include "__mrtrix_plugin.h"

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
  NULL
};

void usage ()
{
  AUTHOR = "Max Pietsch (maximilian.pietsch@kcl.ac.uk)";

  SYNOPSIS = "Perform calculations on linear transformation matrices";

  ARGUMENTS
  + Argument ("input", "the input for the specified operation").allow_multiple()
  + Argument ("operation", "the operation to perform, one of: " + join(operations, ", ") + "."
    + "\n\ninvert: invert the input transformation:\nmatrix_in invert output"

    + "\n\nhalf: calculate the matrix square root of the input transformation:\nmatrix_in half output"

    + "\n\nrigid: calculate the rigid transformation of the affine input transformation:\nmatrix_in rigid output"

    + "\n\nheader: calculate the transformation matrix from an original image and an image with modified header:\nmov mapmovhdr header output"

    + "\n\naverage: calculate the average affine matrix of all input matrices:\ninput ... average output"

    + "\n\ninterpolate: create interpolated transformation matrix between input (t=0) and input2 (t=1). "
        "Based on matrix decomposition with linear interpolation of "
        " translation, rotation and stretch described in "
        " Shoemake, K., Hill, M., & Duff, T. (1992). Matrix Animation and Polar Decomposition. "
        " Matrix, 92, 258-264. doi:10.1.1.56.1336"
        "\ninput input2 interpolate output"

    + "\n\ndecompose: decompose transformation matrix M into translation, rotation and stretch and shear (M = T * R * S). "
        "The output is a key-value text file "
        "scaling: vector of 3 scaling factors in x, y, z direction, "
        "shear: list of shear factors for xy, xz, yz axes, "
        "angles: list of Euler angles about static x, y, z axes in radians in the range [0:pi]x[-pi:pi]x[-pi:pi], "
        "angle_axis: angle in radians and rotation axis, "
        "translation : translation vector along x, y, z axes in mm, "
        "R: composed roation matrix (R = rot_x * rot_y * rot_z), "
        "S: composed scaling and shear matrix."
        "\nmatrix_in decompose output"
    ).type_choice (operations)
  + Argument ("output", "the output transformation matrix.").type_file_out ();
}

template <typename T> int sgn(T val) {
    return (T(0) < val) - (val < T(0));
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

      break;
    }
    default: assert (0);
  }


}
