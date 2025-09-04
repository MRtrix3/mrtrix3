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

#include "command.h"

#include "filter/optimal_threshold.h"
#include "image.h"
#include "surface/algo/image2mesh.h"
#include "surface/mesh.h"

using namespace MR;
using namespace App;

// clang-format off
void usage() {

  AUTHOR = "Robert E. Smith (robert.smith@florey.edu.au)";

  SYNOPSIS = "Generate a surface mesh representation from a voxel image";

  DESCRIPTION +
    "This command utilises the Marching Cubes algorithm"
    " to generate a polygonal surface that represents the isocontour(s) of the input image"
    " at a particular intensity."
    " By default, "
    " an appropriate threshold will be determined automatically from the input image,"
    " however the intensity value of the isocontour(s) can instead be set manually"
    " using the -threhsold option."

  + "If the -blocky option is used,"
    " then the Marching Cubes algorithm will not be used."
    " Instead, the input image will be interpreted as a binary mask image,"
    " and polygonal surfaces will be generated at the outer faces"
    " of the voxel clusters within the mask.";

  ARGUMENTS
  + Argument ("input", "the input image.").type_image_in()
  + Argument ("output", "the output mesh file.").type_file_out();

  OPTIONS
  + Option ("blocky", "generate a \'blocky\' mesh that precisely represents the voxel edges")

  + Option ("threshold", "manually set the intensity threshold for the Marching Cubes algorithm")
    + Argument ("value").type_float();

}
// clang-format on

void run() {

  Surface::Mesh mesh;

  if (get_options("blocky").empty()) {
    auto input = Image<float>::open(argument[0]);
    auto opt = get_options("threshold");
    const float threshold = opt.empty() ? Filter::estimate_optimal_threshold(input) : default_type(opt[0][0]);
    Surface::Algo::image2mesh_mc(input, mesh, threshold);
  } else {
    auto input = Image<bool>::open(argument[0]);
    Surface::Algo::image2mesh_blocky(input, mesh);
  }

  mesh.save(argument[1]);
}
