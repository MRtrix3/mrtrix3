/*
    Copyright 2011 Brain Research Institute, Melbourne, Australia

    Written by Robert E. Smith, 2015.

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


#include "args.h"
#include "command.h"
#include "image/header.h"
#include "mesh/mesh.h"



using namespace MR;
using namespace App;



const OptionGroup transform_options = OptionGroup ("Options for applying spatial transformations to vertices")

  + Option ("transform_first2real", "transform vertices from FSL FIRST's native corrdinate space to real space")
    + Argument ("image").type_image_in()

  + Option ("transform_voxel2real", "transform vertices from voxel space to real space")
    + Argument ("image").type_image_in()

  + Option ("transform_real2voxel", "transform vertices from real space to voxel space")
    + Argument ("image").type_image_in();




void usage ()
{

	AUTHOR = "Robert E. Smith (r.smith@brain.org.au)";

  DESCRIPTION
  + "convert meshes between different formats, and apply transformations.";

  ARGUMENTS
  + Argument ("input",  "the input mesh file").type_file_in()
  + Argument ("output", "the output mesh file").type_file_out();

  OPTIONS
  + Option ("binary", "write the output file in binary format")

  + Option ("smooth", "apply a feature-preserving mesh smoothing algorithm")
    + Argument ("spatial_factor").type_float (0.0f, 10.0f, 1e6)
    + Argument ("influence_factor").type_float (0.0f, 10.0f, 1e6)

  + transform_options;

};



void run ()
{

  // Read in the mesh data
  Mesh::Mesh mesh (argument[0]);

  bool have_transformed = false;

  Options opt = get_options ("transform_first2real");
  if (opt.size()) {
    Image::Header H (opt[0][0]);
    mesh.transform_first_to_realspace (H);
    have_transformed = true;
  }

  opt = get_options ("transform_voxel2real");
  if (opt.size()) {
    if (have_transformed)
      throw Exception ("meshconvert can only perform one spatial transformation per call");
    Image::Header H (opt[0][0]);
    mesh.transform_voxel_to_realspace (H);
    have_transformed = true;
  }

  // Apply smoothing if requested - always done in real space
  opt = get_options ("smooth");
  if (opt.size())
    mesh.smooth (opt[0][0], opt[0][1]);

  opt = get_options ("transform_real2voxel");
  if (opt.size()) {
    if (have_transformed)
      throw Exception ("meshconvert can only perform one spatial transformation per call");
    Image::Header H (opt[0][0]);
    mesh.transform_realspace_to_voxel (H);
    have_transformed = true;
  }

  // Create the output file
  mesh.save (argument[1], get_options ("binary").size());

}
