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


#include "command.h"
#include "header.h"
#include "mesh/mesh.h"



using namespace MR;
using namespace App;
using namespace MR::Mesh;



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

  + transform_options;

};



void run ()
{

  // Read in the mesh data
  MeshMulti meshes;
  try {
    MR::Mesh::Mesh mesh (argument[0]);
    meshes.push_back (mesh);
  } catch (...) {
    meshes.load (argument[0]);
  }

  bool have_transformed = false;

  auto opt = get_options ("transform_first2real");
  if (opt.size()) {
    Header H = Header::open (opt[0][0]);
    for (auto i = meshes.begin(); i != meshes.end(); ++i)
      i->transform_first_to_realspace (H);
    have_transformed = true;
  }

  opt = get_options ("transform_voxel2real");
  if (opt.size()) {
    if (have_transformed)
      throw Exception ("meshconvert can only perform one spatial transformation per call");
    Header H = Header::open (opt[0][0]);
    for (auto i = meshes.begin(); i != meshes.end(); ++i)
      i->transform_voxel_to_realspace (H);
    have_transformed = true;
  }

  opt = get_options ("transform_real2voxel");
  if (opt.size()) {
    if (have_transformed)
      throw Exception ("meshconvert can only perform one spatial transformation per call");
    Header H = Header::open (opt[0][0]);
    for (auto i = meshes.begin(); i != meshes.end(); ++i)
      i->transform_realspace_to_voxel (H);
    have_transformed = true;
  }

  // Create the output file
  if (meshes.size() == 1)
    meshes[0].save (argument[1], get_options ("binary").size());
  else
    meshes.save (argument[1]);

}
