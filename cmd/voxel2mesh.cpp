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

#include "image.h"
#include "filter/optimal_threshold.h"
#include "surface/mesh.h"
#include "surface/algo/image2mesh.h"



using namespace MR;
using namespace App;





void usage ()
{
  AUTHOR = "Robert E. Smith (robert.smith@florey.edu.au)";

  SYNOPSIS = "Generate a surface mesh representation from a voxel image";

  DESCRIPTION +
    "This command utilises the Marching Cubes algorithm to generate a polygonal surface "
    "that represents the isocontour(s) of the input image at a particular intensity. By default, "
    "an appropriate threshold will be determined automatically from the input image, however "
    "the intensity value of the isocontour(s) can instead be set manually using the -threhsold "
    "option."

    "If the -blocky option is used, then the Marching Cubes algorithm will not be used. "
    "Instead, the input image will be interpreted as a binary mask image, and polygonal "
    "surfaces will be generated at the outer faces of the voxel clusters within the mask.";

  ARGUMENTS
  + Argument ("input",  "the input image.").type_image_in ()
  + Argument ("output", "the output mesh file.").type_file_out ();

  OPTIONS
  + Option ("blocky", "generate a \'blocky\' mesh that precisely represents the voxel edges")

  + Option ("threshold", "manually set the intensity threshold for the Marching Cubes algorithm")
    + Argument ("value").type_float();
}


void run ()
{

  Surface::Mesh mesh;

  if (get_options ("blocky").size()) {

    auto input = Image<bool>::open (argument[0]);
    Surface::Algo::image2mesh_blocky (input, mesh);

  } else {
    auto input = Image<float>::open (argument[0]);
    const default_type threshold = get_option_value ("threshold", Filter::estimate_optimal_threshold (input));
    Surface::Algo::image2mesh_mc (input, mesh, threshold);

  }

  mesh.save (argument[1]);

}
