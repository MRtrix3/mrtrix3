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
#include "image.h"
#include "registration/warp/utils.h"
#include "registration/warp/compose.h"
#include "registration/warp/convert.h"
#include "adapter/extract.h"


using namespace MR;
using namespace App;

const char* conversion_type[] = {"deformation2displacement","displacement2deformation","warp2deformation","warp2displacement",NULL};


void usage ()
{
  AUTHOR = "David Raffelt (david.raffelt@florey.edu.au)";

  DESCRIPTION
  + "convert between different representations of a non-linear warp. A deformation field is defined as an image where each voxel "
    "defines the corresponding position in the other image (in scanner space coordinates). A displacement field "
    "stores the displacements(in mm) to the other image from the each voxel's position (in scanner space). The warp file is the "
    "5D format output from mrregister, which contains linear transforms, warps and their inverses that map each image to a midway space."; //TODO at link to warp format documentation

  ARGUMENTS
  + Argument ("in", "the input warp image.").type_image_in ()
  + Argument ("out", "the output warp image.").type_image_out ();

  OPTIONS
  + Option ("type", "the conversion type required. Valid choices are: "
                    "deformation2displacement, displacement2deformation, warp2deformation, warp2displacement (Default: deformation2displacement)")
  + Argument ("choice").type_choice (conversion_type)

  + Option ("template", "define a template image when converting a 5Dwarp file (which is defined in the midway space between image 1 & 2). For example to "
                        "generate the deformation field that maps image1 to image2, then supply image2 as the template image")
  + Argument ("image").type_image_in ()

  + Option ("midway_space",
            "to be used only with 5Dwarp2deformation and 5Dwarp2displacement conversion types. The output will only contain the non-linear warp to map an input "
            "image to the midway space (defined by the 5D warp grid). If a linear transform exists in the 5Dwarp file header then it will be composed and included in the output.")

  + Option ("from",
      "to be used only with 5Dwarp2deformation and 5Dwarp2displacement conversion types. Used to define the direction of the desired output field."
      "Use -from 1 to obtain the image1->image2 field and from 2 for image2->image1. Can be used in combination with the -midway_space option to "
      "produce a field that only maps to midway space.")
  +   Argument ("image").type_integer (1,1,2);
}


void run ()
{
  bool midway_space = get_options("midway_space").size() ? true : false;

  std::string template_filename;
  auto opt = get_options ("template");
  if (opt.size())
    template_filename = str(opt[0][0]);

  int from = 1;
  opt = get_options ("from");
  if (opt.size())
    from = opt[0][0];

  int registration_type = 0;
  opt = get_options ("type");
  if (opt.size())
    registration_type = opt[0][0];

  // deformation2displacement
  if (registration_type == 0) {
    if (midway_space)
      WARN ("-midway_space option ignored with deformation2displacement conversion type");
    if (get_options ("template").size())
      WARN ("-template option ignored with deformation2displacement conversion type");
    if (get_options ("from").size())
      WARN ("-from option ignored with deformation2displacement conversion type");

    auto deformation = Image<default_type>::open (argument[0]).with_direct_io (3);
    if (deformation.ndim() != 4)
      throw Exception ("invalid input image. The input deformation field image must be a 4D file.");
    if (deformation.size(3) != 3)
      throw Exception ("invalid input image. The input deformation field image must have 3 volumes (x,y,z) in the 4th dimension.");

    Image<default_type> output = Image<default_type>::create (argument[1], deformation);
    auto displacement = Image<default_type>::scratch (deformation); // create a scratch since deformation2displacement requires direct io
    Registration::Warp::deformation2displacement (deformation, displacement);
    threaded_copy (displacement, output);

  // displacement2deformation
  } else if (registration_type == 1) {
    auto displacement = Image<default_type>::open (argument[0]).with_direct_io (3);
    if (displacement.ndim() != 4)
      throw Exception ("invalid input image. The input displacement field image must be a 4D file.");
    if (displacement.size(3) != 3)
      throw Exception ("invalid input image. The input displacement field image must have 3 volumes (x,y,z) in the 4th dimension.");

    if (midway_space)
      WARN ("-midway_space option ignored with displacement2deformation conversion type");
    if (get_options ("template").size())
      WARN ("-template option ignored with displacement2deformation conversion type");
    if (get_options ("from").size())
      WARN ("-from option ignored with displacement2deformation conversion type");

    Image<default_type> output = Image<default_type>::create (argument[1], displacement);
    auto deformation = Image<default_type>::scratch (displacement); // create a scratch since deformation2displacement requires direct io
    Registration::Warp::displacement2deformation (displacement, deformation);
    threaded_copy (deformation, output);

   // 5Dwarp2deformation & 5Dwarp2displacement
  } else if (registration_type == 2 || registration_type == 3) {

    auto warp = Image<default_type>::open (argument[0]).with_direct_io (3);
    if (warp.ndim() != 5)
      throw Exception ("invalid input image. The input 5D warp field image must be a 5D file.");
    if (warp.size(3) != 3)
      throw Exception ("invalid input image. The input 5D warp field image must have 3 volumes (x,y,z) in the 4th dimension.");
    if (warp.size(4) != 4)
      throw Exception ("invalid input image. The input 5D warp field image must have 4 volumes in the 5th dimension.");

    Image<default_type> warp_output;
    if (midway_space) {
      warp_output = Registration::Warp::compute_midway_deformation (warp, from);
    } else {
      if (!get_options ("template").size())
        throw Exception ("-template option required with 5Dwarp2deformation or 5Dwarp2displacement conversion type");
      auto template_header = Header::open (argument[1]);
      warp_output = Registration::Warp::compute_full_deformation (warp, template_header, from);
    }

    if (registration_type == 3)
      Registration::Warp::deformation2displacement (warp_output, warp_output);
    Image<default_type> output = Image<default_type>::create (argument[1], warp_output);
    threaded_copy (warp_output, output);
  }

}
