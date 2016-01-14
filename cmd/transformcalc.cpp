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
#include "image.h"
#include "file/nifti1_utils.h"
#include "transform.h"
#include <unsupported/Eigen/MatrixFunctions>


using namespace MR;
using namespace App;

void usage ()
{
  DESCRIPTION
  + "edit transformations."

  + "This command's function is to either convert the transformation matrix provided "
    "by FSL's flirt command to a format usable in MRtrix or to interpolate between "
    "two transformation files";

  ARGUMENTS
  + Argument ("output", "the output transformation matrix.").type_file_out ();


  OPTIONS
    + Option ("flirt_import",
        "Convert a transformation matrix produced by FSL's flirt command into a format usable by MRtrix. "
        "You'll need to provide as additional arguments the save NIfTI images that were passed to flirt "
        "with the -in and -ref options.")
    + Argument ("input", "input transformation matrix").type_file_in ()
    + Argument ("in").type_image_in ()
    + Argument ("ref").type_image_in ()

    + Option ("invert",
      "invert the input transformation.")
    + Argument ("input", "input transformation matrix").type_file_in ()

    + Option ("half",
      "output the matrix square root of the input transformation.")
    + Argument ("input", "input transformation matrix").type_file_in ()

    + Option ("surfer_vox2vox",
        "Convert a transformation matrix produced by freesurfer's robust_register command into a format usable by MRtrix. ")
    + Argument ("vox2vox", "input transformation matrix").type_file_in ()
    + Argument ("mov").type_image_in ()
    + Argument ("dst").type_image_in ()

    + Option ("header",
        "Calculate the transformation matrix from an original image and an image with modified header.")
    + Argument ("mov").type_image_in ()
    + Argument ("mapmovhdr").type_image_in ()

    + Option ("interpolate",
        "Create interpolated transformation matrix between input (t=0) and input2 (t=1). "
        "Based on matrix decomposition with linear interpolation of "
        " translation, rotation and stretch described in "
        " Shoemake, K., Hill, M., & Duff, T. (1992). Matrix Animation and Polar Decomposition. "
        " Matrix, 92, 258-264. doi:10.1.1.56.1336")
    + Argument ("input", "input transformation matrix").type_file_in ()
    + Argument ("input2").type_file_in ()
    + Argument ("t").type_float ();
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
  auto flirt_opt = get_options ("flirt_import");
  auto interp_opt = get_options ("interpolate");
  auto invert_opt = get_options ("invert");
  auto half_opt = get_options ("invert");
  auto surfer_vox2vox_opt = get_options ("surfer_vox2vox");
  auto from_header_opt = get_options ("header");

  size_t options = 0;
  if (flirt_opt.size())
    options++;
  if (invert_opt.size())
    options++;
  if (half_opt.size())
    options++;
  if (interp_opt.size())
    options++;
  if (from_header_opt.size())
    options++;
  if (surfer_vox2vox_opt.size())
    options++;
  if (options != 1)
    throw Exception ("You must specify one option");

  if(invert_opt.size()){
    transform_type input = load_transform<double> (invert_opt[0][0]);
    save_transform (input.inverse(), argument[0]);
  }

  if(half_opt.size()){
    transform_type input = load_transform<double> (half_opt[0][0]);
    Eigen::Matrix<default_type, 4, 4> half;
    half.row(3) << 0, 0, 0, 1.0;
    half.topLeftCorner(3,4) = input.matrix().topLeftCorner(3,4);
    input.matrix() = half.sqrt().topLeftCorner(3,4);
    save_transform (input, argument[0]);
  }

  if(from_header_opt.size()){
    auto orig_header = Header::open (from_header_opt[0][0]);
    auto modified_header = Header::open (from_header_opt[0][1]);

    transform_type forward_transform = Transform(modified_header).voxel2scanner * Transform(orig_header).voxel2scanner.inverse();
    save_transform (forward_transform.inverse(), argument[0]);
  }

  if(surfer_vox2vox_opt.size()){
    auto vox2vox = parse_surfer_transform (surfer_vox2vox_opt[0][0]);

    auto src_header = Header::open (surfer_vox2vox_opt[0][1]);
    auto dest_header = Header::open (surfer_vox2vox_opt[0][2]);

    auto transform_source = Transform(src_header); // .transform();
    auto transform_dest = Transform(dest_header); //dest_header.transform();

    VAR(transform_source.voxel2scanner.matrix());
    VAR(transform_dest.voxel2scanner.matrix());
    VAR(vox2vox.matrix());

    auto forward_transform =  transform_source.voxel2scanner *
      vox2vox.inverse() * transform_dest.voxel2scanner;
    save_transform (forward_transform.inverse(), argument[0]);
    throw Exception ("FIXME: surfer_vox2vox not implemented yet");
  }

  if (flirt_opt.size()) {
    transform_type transform = load_transform<float> (flirt_opt[0][0]);
    if(transform.matrix().topLeftCorner<3,3>().determinant() == float(0.0))
        WARN ("Transformation matrix determinant is zero. Replace hex with plain text numbers.");

    auto src_header = Header::open (flirt_opt[0][1]);
    transform_type src_flirt_to_scanner = get_flirt_transform (src_header);

    auto dest_header = Header::open (flirt_opt[0][2]);
    transform_type dest_flirt_to_scanner = get_flirt_transform (dest_header);

    transform_type forward_transform = dest_flirt_to_scanner * transform * src_flirt_to_scanner.inverse();
    if (((forward_transform.matrix().array() != forward_transform.matrix().array())).any())
      WARN ("NAN in transformation.");
    save_transform (forward_transform.inverse(), argument[0]);
  }
  if(interp_opt.size()) {
    transform_type transform1 = load_transform<double> (interp_opt[0][0]);
    transform_type transform2 = load_transform<double> (interp_opt[0][1]);
    transform_type transform_out;
    double t = interp_opt[0][2];
    if (t < 0.0 || t > 1.0)
      throw Exception ("t has to be in the interval [0,1]");

    Eigen::MatrixXd M1 = transform1.linear();
    Eigen::MatrixXd M2 = transform2.linear();
    if (sgn(M1.determinant()) != sgn(M2.determinant()))
      WARN("transformation determinants have different signs");

    // transform1.computeRotationScaling (R1, S1);
    Eigen::Matrix3d R1 = transform1.rotation(); // SVD based, polar decomposition should be faster [Higham86]
    Eigen::Matrix3d R2 = transform2.rotation();
    Eigen::Quaterniond Q1(R1);
    Eigen::Quaterniond Q2(R2);
    Eigen::Quaterniond Qout;

    // get stretch (shear becomes roation and stretch)
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
    save_transform (transform_out, argument[0]);
  }
}
