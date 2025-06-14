/* Copyright (c) 2008-2025 the MRtrix3 contributors.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Covered Software is provided under this License on an "as is"
 * basis, without warranty of any kind, either expressed, implied, or
 * statutory, including, without limitation, warranties that the
 * Covered Software is free of defects, merchantable, fit for a
 * particular purpose or non-infringing.
 * See the Mozilla Public License v. 2.0 for more details.
 *
 * For more details, see http://www.mrtrix.org/.
 */

#include "algo/loop.h"
#include "command.h"
#include "debug.h"
#include "image.h"
#include "interp/nearest.h"
#include "math/average_space.h"
#include "registration/transform/initialiser_helpers.h"

#include <filesystem>

using namespace MR;
using namespace App;
using namespace Registration;

const default_type PADDING_DEFAULT = 0.0;
const avgspace_voxspacing_t SPACING_DEFAULT_VALUE = avgspace_voxspacing_t::MEAN_PROJECTION;
const std::string SPACING_DEFAULT_STRING = "mean_projection";

// clang-format off
void usage() {

  AUTHOR = "Maximilian Pietsch (maximilian.pietsch@kcl.ac.uk)";

  SYNOPSIS = "Calculate the average (unbiased) coordinate space of all input images";

  DESCRIPTION
  +"The voxel spacings of the calculated average header can be determined from the set of "
   "input images in one of four different ways, "
   "which may be more or less appropriate in different use cases. "
   "These options vary in two key ways: "
   "1. Projected voxel spacing of the input image in the direction of the average header axes "
   "versus the voxel spacing of the input image axis that is closest to the average header axis; "
   "2. Selecting the minimum of these spacings across input images to maintain maximal "
   "spatial resolution versus the mean across images to produce an unbiased average.";

  ARGUMENTS
  + Argument ("input", "the input image(s).").type_image_in().allow_multiple()
  + Argument ("output", "the output image").type_image_out();

  OPTIONS
  + Option ("padding",
            "boundary box padding in voxels."
            " Default: " + str(PADDING_DEFAULT))
    + Argument ("value").type_float(0.0)
  + Option ("spacing",
            "Method for determination of voxel spacings based on"
            " the set of input images and the average header axes"
            " (see Description)."
            " Valid options are: " + join(avgspace_voxspacing_choices, ",") + ";"
            " default = " + SPACING_DEFAULT_STRING)
    + Argument("type").type_choice(avgspace_voxspacing_choices)
  + Option ("fill", "set the intensity in the first volume of the average space to 1")
  + DataType::options();

}
// clang-format on

void run() {
  const std::filesystem::path first_input_path(argument[0]);
  const std::filesystem::path output_path{argument.back()};

  const size_t num_inputs = argument.size() - 1;

  const default_type p = get_option_value("padding", PADDING_DEFAULT);
  auto padding = Eigen::Matrix<default_type, 4, 1>(p, p, p, 1.0);
  INFO("padding in template voxels: " + str(padding.transpose().head<3>()));
  auto opt = get_options("spacing");
  const avgspace_voxspacing_t spacing = opt.empty() ? SPACING_DEFAULT_VALUE : avgspace_voxspacing_t(int(opt[0][0]));
  const bool fill = !get_options("fill").empty();

  std::vector<Header> headers_in;
  size_t dim(Header::open(first_input_path).ndim());
  if (dim < 3 or dim > 4)
    throw Exception("Please provide 3D or 4D images");
  ssize_t volumes(dim == 3 ? 1 : Header::open(first_input_path).size(3));

  for (size_t i = 0; i != num_inputs; ++i) {
    const std::filesystem::path input_path(argument[i]);
    headers_in.push_back(Header::open(input_path));
    if (fill) {
      if (headers_in.back().ndim() != dim)
        throw Exception("Images do not have the same dimensionality");
      if (dim == 4 and volumes != headers_in.back().size(3))
        throw Exception("Images do not have the same number of volumes");
    }
  }

  std::vector<Eigen::Transform<default_type, 3, Eigen::Projective>> transform_header_with;
  auto H = compute_minimum_average_header(headers_in, transform_header_with, spacing, padding);
  H.datatype() = DataType::Bit;
  if (fill) {
    H.ndim() = dim;
    if (dim == 4)
      H.size(3) = headers_in.back().size(3);
  }
  auto out = Image<bool>::create(output_path, H);
  if (fill) {
    for (auto l = Loop(0, 3)(out); l; ++l)
      out.value() = 1;
    Eigen::Matrix<default_type, 3, 1> centre, vox;
    Registration::Transform::Init::get_geometric_centre(out, centre);
    vox = MR::Transform(out).scanner2voxel * centre;
    INFO("centre scanner: " + str(centre.transpose()));
    for (size_t i = 0; i < 3; ++i)
      vox(i) = std::round(vox(i));
    INFO("centre voxel: " + str(vox.transpose()));
  }
  INFO("average transformation:");
  INFO(str(out.transform().matrix()));
  INFO("average voxel to scanner transformation:");
  INFO(str(MR::Transform(out).voxel2scanner.matrix()));
}
