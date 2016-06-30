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
#include "filter/optimal_threshold.h"
#include "mesh/mesh.h"
#include "mesh/vox2mesh.h"



using namespace MR;
using namespace App;





void usage ()
{
  AUTHOR = "Robert E. Smith (robert.smith@florey.edu.au)";

  DESCRIPTION
  + "Generate a mesh file from an image.";

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

  Mesh::Mesh mesh;

  if (get_options ("blocky").size()) {

    auto input = Image<bool>::open (argument[0]);
    Mesh::vox2mesh (input, mesh);

  } else {

    auto input = Image<float>::open (argument[0]);
    float threshold = get_option_value ("threshold", Filter::estimate_optimal_threshold (input));
    Mesh::vox2mesh_mc (input, threshold, mesh);

  }

  mesh.save (argument[1]);

}
