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
        "convert a transformation matrix produced by FSL's flirt command into a format usable by MRtrix. "
        "You'll need to provide as additional arguments the save NIfTI images that were passed to flirt "
        "with the -in and -ref options.")
    + Argument ("in").type_image_in ()
    + Argument ("ref").type_image_in ();
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



void run ()
{
  auto flirt_opt = get_options ("flirt_import");

  if (flirt_opt.size()) {
    transform_type transform = load_transform<float> (argument[0]);
    if (transform.matrix().topLeftCorner<3,3>().determinant() == float(0.0))
        WARN ("Transformation matrix determinant is zero. Replace hex with plain text numbers.");

    auto src_header = Header::open (flirt_opt[0][0]);
    transform_type src_flirt_to_scanner = get_flirt_transform (src_header);

    auto dest_header = Header::open (flirt_opt[0][1]);
    transform_type dest_flirt_to_scanner = get_flirt_transform (dest_header);

    transform_type forward_transform = dest_flirt_to_scanner * transform * src_flirt_to_scanner.inverse();
    if (((forward_transform.matrix().array() != forward_transform.matrix().array())).any())
      WARN ("NAN in transformation.");
    save_transform (forward_transform.inverse(), argument[1]);
  } else {
    throw Exception ("you must supply the in and ref images using the -flirt_import option");
  }
}


