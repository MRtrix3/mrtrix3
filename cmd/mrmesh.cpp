/* Copyright (c) 2008-2017 the MRtrix3 contributors.
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

  SYNOPSIS = "Generate a mesh file from an image";

  ARGUMENTS
  + Argument ("input",  "the input image.").type_image_in ()
  + Argument ("output", "the output mesh file.").type_file_out ();

  OPTIONS
  + Option ("blocky", "generate a \'blocky\' mesh that precisely represents the voxel edges")

  + Option ("threshold", "manually set the intensity threshold at which the mesh will be generated "
                         "(if omitted, a threshold will be determined automatically)")
    + Argument ("value").type_float();
}


void run ()
{

  Surface::Mesh mesh;

  if (get_options ("blocky").size()) {

    auto input = Image<bool>::open (argument[0]);
    Surface::Algo::image2mesh_blocky (input, mesh);

  } else {
    default_type threshold = 0.0;
    auto input = Image<float>::open (argument[0]);
    auto opt = get_options("threshold");
    if ( opt.size() ) {
      threshold = (default_type) opt[0][0];
    } else {
      threshold = Filter::estimate_optimal_threshold (input);
    }
    Surface::Algo::image2mesh_mc (input, mesh, threshold);

  }

  mesh.save (argument[1]);

}
