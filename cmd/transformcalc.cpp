/*
    Copyright 2008 Brain Research Institute, Melbourne, Australia

    Written by J-Donald Tournier, 27/06/08.

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

#include "command.h"
#include "math/math.h"
#include "image.h"
#include "file/nifti1_utils.h"


using namespace MR;
using namespace App;

void usage ()
{
  DESCRIPTION
  + "edit transformations."

  + "Currently, this command's only function is to convert the transformation matrix provided "
    "by FSL's flirt command to a format usable in MRtrix.";

  ARGUMENTS
  + Argument ("input", "input transformation matrix").type_file_in ()
  + Argument ("output", "the output transformation matrix.").type_file_out ();


  OPTIONS
    + Option ("flirt_import", 
        "Convert a transformation matrix produced by FSL's flirt command into a format usable by MRtrix. "
        "You'll need to provide as additional arguments the save NIfTI images that were passed to flirt "
        "with the -in and -ref options.")
    + Argument ("in").type_image_in ()
    + Argument ("ref").type_image_in ()

    + Option ("interpolate",
        "Create interpolated transformation matrix between input (t=0) and input2 (t=1). "
        "Based on matrix decomposition with linear interpolation of "
        " translation, rotation and stretch described in "
        " Shoemake, K., Hill, M., & Duff, T. (1992). Matrix Animation and Polar Decomposition. "
        " Matrix, 92, 258-264. doi:10.1.1.56.1336")
    + Argument ("input2").type_file_in ()
    + Argument ("t").type_float ();
}


transform_type get_flirt_transform (const Header& header)
{
  std::vector<size_t> axes;
  transform_type nifti_transform = File::NIfTI::adjust_transform (header, axes);
  if (nifti_transform.matrix().determinant() < 0.0)
    return nifti_transform;
  transform_type coord_switch;
  coord_switch.setIdentity();
  coord_switch(0,0) = -1.0f;
  coord_switch(0,3) = (header.size(axes[0])-1) * header.spacing(axes[0]);
  return nifti_transform * coord_switch;
}

template <typename T> int sgn(T val) {
    return (T(0) < val) - (val < T(0));
}



void run ()
{
  auto flirt_opt = get_options ("flirt_import");
  auto interp_opt = get_options ("interpolate");

  if (flirt_opt.size() && !interp_opt.size()) {
    transform_type transform = load_transform<float> (argument[0]);
    if(transform.matrix().determinant() == float(0.0))
        WARN ("Transformation matrix determinant is zero. Replace hex with plain text numbers.");

    auto src_header = Header::open (flirt_opt[0][0]);
    transform_type src_flirt_to_scanner = get_flirt_transform (src_header);

    auto dest_header = Header::open (flirt_opt[0][1]);
    transform_type dest_flirt_to_scanner = get_flirt_transform (dest_header);

    transform_type forward_transform = dest_flirt_to_scanner * transform * src_flirt_to_scanner.inverse();
    if (((forward_transform.matrix().array() != forward_transform.matrix().array())).any())
      WARN ("NAN in transformation.");
    save_transform (forward_transform.inverse(), argument[1]);
  } else if(interp_opt.size() && !flirt_opt.size()) {
    transform_type transform1 = load_transform<double> (argument[0]);
    transform_type transform2 = load_transform<double> (interp_opt[0][0]);
    transform_type transform_out;
    double t = interp_opt[0][1];
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
  } else {
    throw Exception ("you must specify either -flirt_import or -interpolate option");
  }
}