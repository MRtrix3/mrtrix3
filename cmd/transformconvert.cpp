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
#include "image.h"
#include "file/nifti1_utils.h"
#include "transform.h"
#include "file/key_value.h"

using namespace MR;
using namespace App;

const char* operations[] = {
  "flirt_import",
  "itk_import",
  NULL
};

void usage ()
{
  AUTHOR = "Max Pietsch (maximilian.pietsch@kcl.ac.uk)";

  DESCRIPTION
  + "This command's function is to convert linear transformation matrices."

  + "It allows to convert the transformation matrix provided by FSL's flirt command "
  + "and ITK's linear transformation format to a format usable in MRtrix."
  ;

  ARGUMENTS
  + Argument ("input", "the input for the specified operation").allow_multiple()
  + Argument ("operation", "the operation to perform, one of:\n" + join(operations, ", ") + "."
    + "\n\nflirt_import: "
    + "Convert a transformation matrix produced by FSL's flirt command into a format usable by MRtrix. "
        "You'll need to provide as additional arguments the NIfTI images that were passed to flirt "
        "with the -in and -ref options:\nmatrix_in in ref flirt_import output"

    + "\n\nitk_import: "
    + "Convert a plain text transformation matrix file produced by ITK's (ANTS, Slicer) affine registration "
    + "into a format usable by MRtrix."
    ).type_choice (operations)

  + Argument ("output", "the output transformation matrix.").type_file_out ();
}


transform_type get_flirt_transform (const Header& header) {
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

// transform_type parse_surfer_transform (const Header& header) {
//   // TODO
//   return transform_type();
// }

// //! read matrix data into a 2D vector \a filename
// template <class ValueType = default_type>
//   transform_type parse_surfer_transform (const std::string& filename) {
//     std::ifstream stream (filename, std::ios_base::in | std::ios_base::binary);
//     std::vector<std::vector<ValueType>> V;
//     std::string sbuf;
//     std::string file_version;
//     while (getline (stream, sbuf)) {
//       if (sbuf.substr (0,1) == "#")
//         continue;
//       if (sbuf.find("1 4 4") != std::string::npos)
//         break;
//       if (sbuf.find("type") == std::string::npos)
//         continue;
//       file_version = sbuf.substr (sbuf.find_first_of ('=')+1);
//     }
//     if (file_version.empty())
//       throw Exception ("not a surfer transformation");
//     // so far we only understand vox2vox transformations
//     // vox2vox = inverse(colrowslice_to_xyz(moving))*M*colrowslice_to_xyz(target)
//     if (stoi(file_version)!=0)
//       throw Exception ("not a vox2vox transformation");

//     for (auto i = 0; i<4 && getline (stream, sbuf); ++i){
//       // sbuf = strip (sbuf.substr (0, sbuf.find_first_of ('type')));
//       V.push_back (std::vector<ValueType>());
//       const auto elements = MR::split (sbuf, " ,;\t", true);
//       for (const auto& entry : elements)
//         V.back().push_back (to<ValueType> (entry));
//       if (V.size() > 1)
//         if (V.back().size() != V[0].size())
//           throw Exception ("uneven rows in matrix");
//     }
//     if (stream.bad())
//       throw Exception (strerror (errno));
//     if (!V.size())
//       throw Exception ("no data in file");

//     transform_type M;
//     for (ssize_t i = 0; i < 3; i++)
//       for (ssize_t j = 0; j < 4; j++)
//         M(i,j) = V[i][j];
//     return M;
//   }

// template <typename T> int sgn(T val) {
//     return (T(0) < val) - (val < T(0));
// }

template <typename TransformationType>
void parse_itk_trafo (const std::string& itk_file, TransformationType& transformation, Eigen::Vector3d& centre_of_rotation) {
  const std::string first_line = "#Insight Transform File V1.0";
  std::vector<std::string> supported_transformations = {"MatrixOffsetTransformBase_double_3_3",
                                                        "MatrixOffsetTransformBase_float_3_3",
                                                        "AffineTransform_double_3_3",
                                                        "AffineTransform_float_3_3"
                                                      };
  // TODO, support derived classes that are compatible
  // FixedCenterOfRotationAffineTransform_float_3_3?
  // QuaternionRigidTransform_double_3_3?
  // QuaternionRigidTransform_float_3_3?

  File::KeyValue file (itk_file, first_line.c_str());
  std::string line;
  size_t invalid (2);
  while (file.next()) {
    if (file.key() == "Transform") {
      if (std::find(supported_transformations.begin(), supported_transformations.end(), file.value()) == supported_transformations.end())
        throw Exception ("The " + file.value() + " transform type is currenly not supported or tested");
    }
    else if (file.key() == "Parameters") {
      line = file.value();
      std::replace (line.begin(), line.end(), ' ', ',');
      std::vector<default_type> parameters (parse_floats (line));
      if (parameters.size() != 12)
        throw Exception ("Expected itk file with 12 parameters but has " + str(parameters.size()) + " parameters.");
      transformation.linear().row(0) << parameters[0], parameters[1], parameters[2];
      transformation.linear().row(1) << parameters[3], parameters[4], parameters[5];
      transformation.linear().row(2) <<  parameters[6], parameters[7], parameters[8];
      transformation.translation() << parameters[9], parameters[10], parameters[11];
      invalid--;
    }
    else if (file.key() == "FixedParameters") {
      line = file.value();
      std::replace (line.begin(), line.end(), ' ', ',');
      std::vector<default_type> fixed_parameters (parse_floats (line));
      centre_of_rotation << fixed_parameters[0], fixed_parameters[1], fixed_parameters[2];
      invalid--;
    }
  }
  file.close();
  if (invalid != 0)
    throw Exception ("ITK transformation could not be read");
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
      transform_type transform = load_transform (argument[0]);
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
    case 1: { // ITK import
      if (num_inputs != 1)
        throw Exception ("itk_import requires 1 input, " + str(num_inputs) + " provided.");

      transform_type transform;
      Eigen::Vector3d centre_of_rotation (3);
      parse_itk_trafo (argument[0], transform, centre_of_rotation);
      INFO("Centre of rotation:\n" + str(centre_of_rotation.transpose()));

      // rejig translation to correct for centre of rotation
      transform.translation() = transform.translation() + centre_of_rotation - transform.linear() * centre_of_rotation;

      // TODO is the coordinate switch robust to large rotations?
      transform.matrix().template block<2,2>(0,2) *= -1.0;
      transform.matrix().template block<1,2>(2,0) *= -1.0;

      INFO("linear:\n" + str(transform.matrix()));
      INFO("translation:\n" + str(transform.translation().transpose()));
      if (((transform.matrix().array() != transform.matrix().array())).any())
        WARN ("NAN in transformation.");

      save_transform (transform, output_path);
      break;
    }
    default: assert (0);
  }


}
