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


#include <unsupported/Eigen/MatrixFunctions>
#include <algorithm>
#include "command.h"
#include "math/math.h"
#include "math/average_space.h"
#include "image.h"
#include "file/nifti1_utils.h"
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
  NULL
};

void usage ()
{
  AUTHOR = "Max Pietsch (maximilian.pietsch@kcl.ac.uk)";

  DESCRIPTION
  + "This command's function is to process linear transformation matrices."

  + "It allows to perform affine matrix operations or to convert the transformation matrix provided by FSL's flirt command to a format usable in MRtrix"
  ;

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
      transform_type input = load_transform<default_type> (argument[0]);
      save_transform (input.inverse(), output_path);
      break;
    }
    case 1: { // half
      if (num_inputs != 1)
        throw Exception ("half requires 1 input");
      Eigen::Transform<default_type, 3, Eigen::Projective> input = load_transform<default_type> (argument[0]);
      transform_type output;
      Eigen::Matrix<default_type, 4, 4> half = input.matrix().sqrt();
      output.matrix() = half.topLeftCorner(3,4);
      save_transform (output, output_path);
      break;
    }
    case 2: { // rigid
      if (num_inputs != 1)
        throw Exception ("rigid requires 1 input");
      transform_type input = load_transform<default_type> (argument[0]);
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
      std::vector<Eigen::MatrixXd> matrices;
      for (size_t i = 0; i < num_inputs; i++) {
        DEBUG(str(argument[i]));
        Tin = load_transform<default_type> (argument[i]);
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
      transform_type transform1 = load_transform<default_type> (argument[0]);
      transform_type transform2 = load_transform<default_type> (argument[1]);
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
    default: assert (0);
  }


}
