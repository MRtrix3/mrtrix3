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


const char* conversion_type[] = { "deformation2displacement", "displacement2deformation", "5Dwarp2deformation", "5Dwarp2displacement", NULL };


void usage ()
{
  AUTHOR = "David Raffelt (david.raffelt@florey.edu.au)";

  DESCRIPTION
  + "convert between different representations of a non-linear warp. Where a deformation field is defined as an image where each voxel "
    "(containing x,y,z components) defines the corresponding position in the other image (in scanner space coordinates). A displacement field "
    "stores the offsets (displacements)(in mm) to the other image from the current voxels position (in scanner space). The 5D warp file is the "
    "format output from mrregister, which contains linear transforms, warps and their inverses that map each image to a midway space."; //TODO at link to warp format documentation

  ARGUMENTS
  + Argument ("in", "the input warp image.").type_image_in ()
  + Argument ("out", "the output warp image.").type_image_out ();

  OPTIONS
  + Option ("type", "the conversion type required. Valid choices are: "
                    "deformation2displacement, displacement2deformation, 5Dwarp2deformation, 5Dwarp2displacement (Default: deformation2displacement)")
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

  Image<default_type> output;

  switch (registration_type) {

    case 0: // deformation2displacement
      {
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

        output = Image<default_type>::create (argument[1], deformation);
        auto displacement = Image<default_type>::scratch (deformation); // create a scratch since deformation2displacement requires direct io
        Registration::Warp::deformation2displacement (deformation, displacement);
        threaded_copy (displacement, output);
      }
      break;

    case 1: // displacement2deformation
      {
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

        output = Image<default_type>::create (argument[1], displacement);
        auto deformation = Image<default_type>::scratch (displacement); // create a scratch since deformation2displacement requires direct io
        Registration::Warp::displacement2deformation (displacement, deformation);
        threaded_copy (deformation, output);
      }
      break;

    case 2: // 5Dwarp2deformation
    {
        if (!get_options ("template").size())
          throw Exception ("-template option required with 5Dwarp2deformation conversion type");

        auto template_header = Header::open (argument[1]);
        auto warp = Image<default_type>::open (argument[0]).with_direct_io (3);

        if (warp.ndim() != 5)
          throw Exception ("invalid input image. The input 5D warp field image must be a 5D file.");
        if (warp.size(4) != 3)
          throw Exception ("invalid input image. The input 5D warp field  image must have 3 volumes (x,y,z) in the 5th dimension.");
        if (warp.size(3) != 4)
          throw Exception ("invalid input image. The input 5D warp field  image must have 4 volumes in the 4th dimension.");

        Header deform_header (template_header);
        deform_header.datatype() = DataType::Float32;
        deform_header.set_ndim(4);
        deform_header.size(3) = 3;

        auto output = Image<default_type>::create (argument[1], deform_header);
        Image<default_type> warp_deform = Image<default_type>::scratch (deform_header);

        transform_type linear1 = Registration::Warp::parse_linear_transform (warp, "linear1");
        transform_type linear2 = Registration::Warp::parse_linear_transform (warp, "linear2");

        std::vector<int> index(1);
        if (from == 1) {
          index[0] = 0;
          Adapter::Extract1D<Image<default_type>> displacement1 (warp, 4, index);
          index[0] = 3;
          Adapter::Extract1D<Image<default_type>> displacement2 (warp, 4, index);
          Registration::Warp::compose_halfway_transforms ("converting warp", linear2.inverse(), displacement2, displacement1, linear1, warp_deform);
        } else {
          index[0] = 1;
          Adapter::Extract1D<Image<default_type>> displacement1 (warp, 4, index);
          index[0] = 2;
          Adapter::Extract1D<Image<default_type>> displacement2 (warp, 4, index);
          Registration::Warp::compose_halfway_transforms ("converting warp", linear1.inverse(), displacement1, displacement2, linear2, warp_deform);
        }
        threaded_copy (warp_deform, output);
      }
      break;

    case 3: // 5Dwarp2displacement
      {

        if (!get_options ("template").size())
          throw Exception ("-template option required with 5Dwarp2displacement conversion type");

      }
    break;
    default:
    break;

  }



}
