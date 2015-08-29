/*
    Copyright 2012 Brain Research Institute, Melbourne, Australia

    Written by Robert E. Smith, 09/04/2015.

    This file is part of MRtrix.

    MRtrix is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    MRtrix is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with MRtrix.  If not, see <http://www.gnu.org/licenses/>.

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
  AUTHOR = "Robert E. Smith (r.smith@brain.org.au)";

  DESCRIPTION
  + "Generate a mesh file from an image.";


  ARGUMENTS
  + Argument ("input",  "the input image.").type_image_in ()
  + Argument ("output", "the output mesh file.").type_file_out ();

  OPTIONS
  + Option ("blocky", "generate a \'blocky\' mesh that precisely represents the voxel edges")

  + Option ("threshold", "manually set the intensity threshold at which the mesh will be generated "
                         "(if omitted, a threshold will be determined automatically)")
    + Argument ("value").type_float (-std::numeric_limits<float>::max(), 0.0f, std::numeric_limits<float>::max());
}


void run ()
{

  Mesh::Mesh mesh;

  if (get_options ("blocky").size()) {

    auto input = Image<bool>::open (argument[0]);
    Mesh::vox2mesh (input, mesh);

  } else {

    auto input = Image<float>::open (argument[0]);
    float threshold;
    auto opt = get_options ("threshold");
    if (opt.size())
      threshold = opt[0][0];
    else
      threshold = Filter::estimate_optimal_threshold (input);
    Mesh::vox2mesh_mc (input, threshold, mesh);

  }

  mesh.save (argument[1]);

}
