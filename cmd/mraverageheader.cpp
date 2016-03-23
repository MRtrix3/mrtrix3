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
#include "debug.h"
#include "image.h"
#include "image/average_space.h"
// #include "registration/transform/initialiser.h"
// Registration::Transform::Init::LinearInitialisationParams init;
#include "registration/transform/initialiser_helpers.h"
#include "interp/nearest.h"


using namespace MR;
using namespace App;
using namespace Registration;

default_type PADDING_DEFAULT     = 0.0;
default_type TEMPLATE_RESOLUTION = 0.9;


void usage ()
{
  AUTHOR = "Maximilian Pietsch (maximilian.pietsch@kcl.ac.uk)";

  DESCRIPTION
  + "This command calculates the average (unbiased) coordinate space of all input images";

  ARGUMENTS
  + Argument ("input", "the input image(s).").type_image_in ().allow_multiple()
  + Argument ("output", "the output image").type_image_out ();

  OPTIONS
  + Option ("padding", " boundary box padding in voxels. Default: " + str(PADDING_DEFAULT))
  +   Argument ("value").type_float (0.0, PADDING_DEFAULT, std::numeric_limits<default_type>::infinity())
  + Option ("template_res", " subsampling of template compared to smallest voxel size in any input image. Default: " + str(TEMPLATE_RESOLUTION))
  +   Argument ("value").type_float (default_type(0.0), TEMPLATE_RESOLUTION, default_type(1.0))
  + Option ("mark_centre", " set intensity in central voxel of average space to 1")
  + DataType::options();
}


void run ()
{

    const size_t num_inputs = argument.size()-1;

    auto opt = get_options ("padding");
    const default_type p = opt.size() ? default_type(opt[0][0]) : PADDING_DEFAULT;
    auto padding = Eigen::Matrix<default_type, 4, 1>(p, p, p, 1.0);
    INFO("padding in template voxels: " + str(padding.transpose().head<3>()));

    opt = get_options ("template_res");
    const default_type template_res = opt.size() ? default_type(opt[0][0]) : TEMPLATE_RESOLUTION;
    INFO("template voxel subsampling: " + str(template_res));

    bool mark_centre = get_options ("mark_centre").size();

    std::vector<Header> headers_in;
    size_t dim (Header::open (argument[0]).ndim());
    if (dim < 3 or dim > 4)
      throw Exception ("Please provide 3D or 4D images");
    size_t volumes (dim == 3 ? 1 : Header::open (argument[0]).size(3));

    for (size_t i = 0; i != num_inputs; ++i) {
        headers_in.push_back (Header::open (argument[i]));
        if (mark_centre) {
          if (headers_in.back().ndim() != dim)
            throw Exception ("Images do not have the same dimensionality");
          if (dim == 4 and volumes != headers_in.back().size(3))
            throw Exception ("Images do not have the same number of volumes");
        }
    }

    auto trafo = headers_in[0].transform();
    std::vector<decltype(trafo)> transform_header_with;

    auto H = compute_minimum_average_header<double,decltype(trafo)>(headers_in, template_res, padding, transform_header_with);
    if (mark_centre) {
      H.set_ndim(dim);
      if (dim == 4)
        H.size(3) = headers_in.back().size(3);
    }
    auto out = Image<bool>::create(argument[argument.size()-1],H);
    if (mark_centre) {
      Eigen::Matrix<default_type, 3, 1> centre, vox;
      Registration::Transform::Init::get_geometric_centre (out, centre);
      vox = MR::Transform(out).scanner2voxel * centre;
      INFO("centre scanner: " + str(centre.transpose()));
      for (size_t i = 0; i < 3; ++i)
        vox(i) = std::round (vox(i));
      INFO("centre voxel: " + str(vox.transpose()));
      out.index(0) = vox(0);
      out.index(1) = vox(1);
      out.index(2) = vox(2);
      out.value() = 1.0;
    }
    INFO("average transformation:");
    INFO(str(out.transform().matrix()));
    auto trafo2 = MR::Transform(out);
    INFO("average voxel to scanner transformation:");
    INFO(str(trafo2.voxel2scanner.matrix()));
}
