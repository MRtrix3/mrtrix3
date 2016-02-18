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

namespace MR {
}


using namespace MR;
using namespace App;

typedef double ComputeType;
typedef Eigen::VectorXd VectorType;
typedef Eigen::RowVectorXd RowVectorType;
ComputeType PADDING_DEFAULT     = 0.0;
ComputeType TEMPLATE_RESOLUTION = 0.9;


void usage ()
{
  AUTHOR = "Joe Bloggs (joe.bloggs@acme.org)";

  DESCRIPTION
  + "test average space calculation";

  ARGUMENTS
  + Argument ("input", "the input image(s).").type_image_in ().allow_multiple()
  + Argument ("output", "the output image").type_image_out ();

  OPTIONS
  + Option ("padding", " template boundary box padding in voxels. Default: " + str(PADDING_DEFAULT))
  +   Argument ("value").type_float (0.0, PADDING_DEFAULT, std::numeric_limits<ComputeType>::infinity())
  + Option ("template_res", " subsampling of template compared to smallest voxel size in any input image. Default: " + str(TEMPLATE_RESOLUTION))
  +   Argument ("value").type_float (ComputeType(0.0), TEMPLATE_RESOLUTION, ComputeType(1.0))
  + DataType::options();
}


void run ()
{

    const size_t num_inputs = argument.size()-1;
    // typedef Eigen::Transform< ComputeType, 3, Eigen::Projective> TransformType;

    auto opt = get_options ("padding");
    const ComputeType p = opt.size() ? ComputeType(opt[0][0]) : PADDING_DEFAULT;
    // VectorType padding = RowVectorType(4);
    auto padding = Eigen::Matrix<ComputeType, 4, 1>(p, p, p, 1.0);
    INFO("padding in template voxels: " + str(padding.transpose().head<3>()));

    opt = get_options ("template_res");
    const ComputeType template_res = opt.size() ? ComputeType(opt[0][0]) : TEMPLATE_RESOLUTION;
    INFO("template voxel subsampling: " + str(template_res));

    std::vector<Header> headers_in;

    for (size_t i = 0; i != num_inputs; ++i) {
        headers_in.push_back (Header::open (argument[i]));
        const Header& temp (headers_in[i]);
        if (temp.ndim() < 3)
            throw Exception ("Please provide 3D or 4D images");
    }

    auto trafo = headers_in[0].transform();
    std::vector<decltype(trafo)> transform_header_with;

    auto H = compute_minimum_average_header<double,decltype(trafo)>(headers_in, template_res, padding, transform_header_with);
    auto out = Header::create(argument[argument.size()-1],H);
    INFO("average transformation:");
    INFO(str(out.transform().matrix()));
    auto trafo2 = Transform(out);
    INFO("average voxel to scanner transformation:");
    INFO(str(trafo2.voxel2scanner.matrix()));
}
