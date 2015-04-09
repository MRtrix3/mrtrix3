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
#include "mesh/mesh.h"
#include "mesh/vox2mesh.h"



using namespace MR;
using namespace App;





void usage ()
{
  AUTHOR = "Robert E. Smith (r.smith@brain.org.au)";

  DESCRIPTION
  + "Generate a mesh file from a mask image.";


  ARGUMENTS
  + Argument ("input",  "the input image.").type_image_in ()
  + Argument ("output", "the output mesh file.").type_file_out ();

}


void run () {

  Image::Buffer<bool> input_data (argument[0]);
  auto input_voxel = input_data.voxel();

  Mesh::Mesh mesh;
  Mesh::vox2mesh (input_voxel, mesh);

  mesh.save (argument[1]);

}
