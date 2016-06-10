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


#include "command.h"
#include "math/math.h"
#include "math/average_space.h"
#include "image.h"
#include "file/nifti1_utils.h"
#include "transform.h"
#include <unsupported/Eigen/MatrixFunctions>


using namespace MR;
using namespace App;

const char* operations[] = {
  "flirt_import",
  "invert",
  "half",
  "rigid",
  "header",
  "average",
  "interpolate",
  NULL
};

// const char description_flirt_import[] = "flirt_import" + "convert the transformation matrix provided by FSL's flirt command to a format usable in MRtrix";

  // VAR(join(description_flirt_import, ", "));
  // VAR(join(description_flirt_import, ", ").c_str());

void usage ()
{
  DESCRIPTION
  + "This command's function is to process linear transformation matrices."

  + "It allows to perform affine matrix operations or to convert the transformation matrix provided by FSL's flirt command to a format usable in MRtrix"
  ;

  ARGUMENTS
  + Argument ("input", "the input for the specified operation").allow_multiple()
  + Argument ("operation", "the operation to perform, one of: " + join(operations, ", ") + "."
    + "\n\nflirt_import: "
    + "Convert a transformation matrix produced by FSL's flirt command into a format usable by MRtrix. "
        "You'll need to provide as additional arguments the save NIfTI images that were passed to flirt "
        "with the -in and -ref options:\nmatrix_in in ref flirt_import output"

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


transform_type get_flirt_transform (const Header& header)
{
  std::vector<size_t> axes;
  transform_type nifti_transform = File::NIfTI::adjust_transform (header, axes);
  if (nifti_transform.matrix().topLeftCorner<3,3>().determinant() < 0.0)
    return nifti_transform;
  transform_type coord_switch;
  coord_switch.setIdentity();
  coord_switch(0,0) = -1.0f;
  coord_switch(0,3) = (header.size(axes[0])-1) * header.spacing(axes[0]);
  return nifti_transform * coord_switch;
}

transform_type get_surfer_transform (const Header& header) {
  // TODO
  return transform_type();
}

//! read matrix data into a 2D vector \a filename
template <class ValueType = default_type>
  transform_type parse_surfer_transform (const std::string& filename) {
    std::ifstream stream (filename, std::ios_base::in | std::ios_base::binary);
    std::vector<std::vector<ValueType>> V;
    std::string sbuf;
    std::string file_version;
    while (getline (stream, sbuf)) {
      if (sbuf.substr (0,1) == "#")
        continue;
      if (sbuf.find("1 4 4") != std::string::npos)
        break;
      if (sbuf.find("type") == std::string::npos)
        continue;
      file_version = sbuf.substr (sbuf.find_first_of ('=')+1);
    }
    if (file_version.empty())
      throw Exception ("not a surfer transformation");
    // so far we only understand vox2vox transformations
    // vox2vox = inverse(colrowslice_to_xyz(moving))*M*colrowslice_to_xyz(target)
    if (stoi(file_version)!=0)
      throw Exception ("not a vox2vox transformation");

    for (auto i = 0; i<4 && getline (stream, sbuf); ++i){
      // sbuf = strip (sbuf.substr (0, sbuf.find_first_of ('type')));
      V.push_back (std::vector<ValueType>());
      const auto elements = MR::split (sbuf, " ,;\t", true);
      for (const auto& entry : elements)
        V.back().push_back (to<ValueType> (entry));
      if (V.size() > 1)
        if (V.back().size() != V[0].size())
          throw Exception ("uneven rows in matrix");
    }
    if (stream.bad())
      throw Exception (strerror (errno));
    if (!V.size())
      throw Exception ("no data in file");

    transform_type M;
    for (ssize_t i = 0; i < 3; i++)
      for (ssize_t j = 0; j < 4; j++)
        M(i,j) = V[i][j];
    return M;
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
    case 0: { // flirt_import
      if (num_inputs != 3)
        throw Exception ("flirt_import requires 3 inputs");
      transform_type transform = load_transform<default_type> (argument[0]);
      auto src_header = Header::open (argument[1]); // -in
      auto dest_header = Header::open (argument[2]); // -ref

      if (transform.matrix().topLeftCorner<3,3>().determinant() == float(0.0))
          WARN ("Transformation matrix determinant is zero.");
      if (transform.matrix().topLeftCorner<3,3>().determinant() < 0)
          INFO ("Transformation matrix determinant is negative.");

      transform_type src_flirt_to_scanner = get_flirt_transform (src_header);
      transform_type dest_flirt_to_scanner = get_flirt_transform (dest_header);

      transform_type forward_transform = dest_flirt_to_scanner * transform * src_flirt_to_scanner.inverse();
      if (((forward_transform.matrix().array() != forward_transform.matrix().array())).any())
        WARN ("NAN in transformation.");
      save_transform (forward_transform.inverse(), output_path);
      break;
    }
    case 1: { // invert
      if (num_inputs != 1)
        throw Exception ("invert requires 1 input");
      transform_type input = load_transform<default_type> (argument[0]);
      save_transform (input.inverse(), output_path);
      break;
    }
    case 2: { // half
      if (num_inputs != 1)
        throw Exception ("half requires 1 input");
      Eigen::Transform<default_type, 3, Eigen::Projective> input = load_transform<default_type> (argument[0]);
      transform_type output;
      Eigen::Matrix<default_type, 4, 4> half = input.matrix().sqrt();
      output.matrix() = half.topLeftCorner(3,4);
      save_transform (output, output_path);
      break;
    }
    case 3: { // rigid
      if (num_inputs != 1)
        throw Exception ("rigid requires 1 input");
      transform_type input = load_transform<default_type> (argument[0]);
      transform_type output (input);
      output.linear() = input.rotation();
      save_transform (output, output_path);
      break;
    }
    case 4: { // header
      if (num_inputs != 2)
        throw Exception ("header requires 2 inputs");
      auto orig_header = Header::open (argument[0]);
      auto modified_header = Header::open (argument[1]);

      transform_type forward_transform = Transform(modified_header).voxel2scanner * Transform(orig_header).voxel2scanner.inverse();
      save_transform (forward_transform.inverse(), output_path);
      break;
    }
    case 5: { // average
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
    case 6: { // interpolate
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
