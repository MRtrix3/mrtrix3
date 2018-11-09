/*
 * Copyright (c) 2008-2018 the MRtrix3 contributors.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, you can obtain one at http://mozilla.org/MPL/2.0/
 *
 * MRtrix3 is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * For more details, see http://www.mrtrix.org/
 */


#include "command.h"
#include "datatype.h"
#include "header.h"
#include "image.h"
#include "registration/warp/helpers.h"
#include "registration/warp/invert.h"


using namespace MR;
using namespace App;

void usage ()
{
  AUTHOR = "Robert E. Smith (robert.smith@florey.edu.au) and David Raffelt (david.raffelt@florey.edu.au)";

  SYNOPSIS = "Invert a non-linear warp field";

  DESCRIPTION
  + "By default, this command assumes that the input warp field is a deformation field, i.e. each voxel "
    "stores the corresponding position in the other image (in scanner space), and the calculated output "
    "warp image will also be a deformation field. If the input warp field is instead a displacment field, "
    "i.e. where each voxel stores an offset from which to sample the other image (but still in scanner "
    "space), then the -displacement option should be used; the output warp field will additionally be "
    "calculated as a displacement field in this case.";

  ARGUMENTS
  + Argument ("in", "the input warp image.").type_image_in ()
  + Argument ("out", "the output warp image.").type_image_out ();

  OPTIONS
  + Option ("template", "define a template image grid for the output warp")
  + Argument ("image").type_image_in ()

  + Option ("displacement", "indicates that the input warp field is a displacement field; the output will also be a displacement field");
}


void run ()
{
  const bool displacement = get_options ("displacement").size();
  Header header_in (Header::open (argument[0]));
  Registration::Warp::check_warp (header_in);
  Header header_out (header_in);
  auto opt = get_options ("template");
  if (opt.size()) {
    header_out = Header::open (opt[0][0]);
    if (displacement) {
      header_out.ndim() = 3;
    } else {
      header_out.ndim() = 4;
      header_out.size(3) = 3;
    }
    header_out.datatype() = DataType::Float32;
    header_out.datatype().set_byte_order_native();
  }

  Image<default_type> image_in (header_in.get_image<default_type>());
  Image<default_type> image_out (Image<default_type>::create (argument[1], header_out));

  if (displacement) {
    Registration::Warp::invert_displacement (image_in, image_out);
  } else {
    Registration::Warp::invert_deformation (image_in, image_out);
  }
}
