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

#include "image/buffer.h"
#include "image/filter/optimal_threshold.h"
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

    Image::Buffer<bool> input_data (argument[0]);
    auto input_voxel = input_data.voxel();
    Mesh::vox2mesh (input_voxel, mesh);

  } else {

    Image::Buffer<float> input_data (argument[0]);
    auto input_voxel = input_data.voxel();
    float threshold;
    Options opt = get_options ("threshold");
    if (opt.size())
      threshold = opt[0][0];
    else
      threshold = Image::Filter::estimate_optimal_threshold (input_voxel);
    Mesh::vox2mesh_mc (input_voxel, threshold, mesh);

  }

  mesh.save (argument[1]);

}
