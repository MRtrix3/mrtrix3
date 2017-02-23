/* Copyright (c) 2008-2017 the MRtrix3 contributors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, you can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * MRtrix is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * For more details, see http://www.mrtrix.org/.
 */
#include "__mrtrix_plugin.h"

#include "command.h"
#include "debug.h"
#include "image.h"
#include "math/average_space.h"
#include "registration/transform/initialiser_helpers.h"
#include "interp/nearest.h"
#include "algo/loop.h"

using namespace MR;
using namespace App;
using namespace Registration;

default_type PADDING_DEFAULT     = 0.0;

enum RESOLUTION {MAX, MEAN};
const char* resolution_choices[] = { "max", "mean", nullptr };

void usage ()
{
  AUTHOR = "Maximilian Pietsch (maximilian.pietsch@kcl.ac.uk)";

  SYNOPSIS = "Calculate the average (unbiased) coordinate space of all input images";

  ARGUMENTS
  + Argument ("input", "the input image(s).").type_image_in ().allow_multiple()
  + Argument ("output", "the output image").type_image_out ();

  OPTIONS
  + Option ("padding", " boundary box padding in voxels. Default: " + str(PADDING_DEFAULT))
  +   Argument ("value").type_float (0.0, std::numeric_limits<default_type>::infinity())
  + Option ("resolution", " subsampling of template compared to smallest voxel size in any input image. "
        "Valid options are 'mean': unbiased but loss of resolution for individual images possible, "
        "and 'max': smallest voxel size of any input image defines the resolution. Default: mean")
  +   Argument ("type").type_choice (resolution_choices)
  + Option ("fill", " set the intensity in the first volume of the average space to 1")
  + DataType::options();
}


void run ()
{

    const size_t num_inputs = argument.size()-1;

    auto opt = get_options ("padding");
    const default_type p = opt.size() ? default_type(opt[0][0]) : PADDING_DEFAULT;
    auto padding = Eigen::Matrix<default_type, 4, 1>(p, p, p, 1.0);
    INFO("padding in template voxels: " + str(padding.transpose().head<3>()));

    opt = get_options ("resolution");
    const int resolution = opt.size() ? int(opt[0][0]) : 1;
    INFO("template voxel subsampling: " + str(resolution));

    bool fill = get_options ("fill").size();

    vector<Header> headers_in;
    size_t dim (Header::open (argument[0]).ndim());
    if (dim < 3 or dim > 4)
      throw Exception ("Please provide 3D or 4D images");
    ssize_t volumes (dim == 3 ? 1 : Header::open (argument[0]).size(3));

    for (size_t i = 0; i != num_inputs; ++i) {
        headers_in.push_back (Header::open (argument[i]));
        if (fill) {
          if (headers_in.back().ndim() != dim)
            throw Exception ("Images do not have the same dimensionality");
          if (dim == 4 and volumes != headers_in.back().size(3))
            throw Exception ("Images do not have the same number of volumes");
        }
    }

    vector<Eigen::Transform<default_type, 3, Eigen::Projective>> transform_header_with;
    auto H = compute_minimum_average_header (headers_in, resolution, padding, transform_header_with);
    H.datatype() = DataType::Bit;
    if (fill) {
      H.ndim() = dim;
      if (dim == 4)
        H.size(3) = headers_in.back().size(3);
    }
    auto out = Image<bool>::create(argument[argument.size()-1], H);
    if (fill) {
      for (auto l = Loop (0,3) (out); l; ++l)
        out.value() = 1;
      Eigen::Matrix<default_type, 3, 1> centre, vox;
      Registration::Transform::Init::get_geometric_centre (out, centre);
      vox = MR::Transform(out).scanner2voxel * centre;
      INFO("centre scanner: " + str(centre.transpose()));
      for (size_t i = 0; i < 3; ++i)
        vox(i) = std::round (vox(i));
      INFO("centre voxel: " + str(vox.transpose()));
    }
    INFO("average transformation:");
    INFO(str(out.transform().matrix()));
    INFO("average voxel to scanner transformation:");
    INFO(str(MR::Transform(out).voxel2scanner.matrix()));
}
