/*
    Copyright 2011 Brain Research Institute, Melbourne, Australia

    Written by Robert E. Smith, 2011.

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

#include "image/header.h"

#include "mesh/mesh.h"





using namespace MR;
using namespace App;



void usage ()
{

	AUTHOR = "Robert E. Smith (r.smith@brain.org.au)";

  DESCRIPTION
  + "convert a mesh surface to a partial volume estimation image.";

  REFERENCES = "Smith, R. E.; Tournier, J.-D.; Calamante, F. & Connelly, A. "
               "Anatomically-constrained tractography: Improved diffusion MRI streamlines tractography through effective use of anatomical information. "
               "NeuroImage, 2012, 62, 1924-1938";

  ARGUMENTS
  + Argument ("source",   "the mesh file (currently only .vtk files are supported)").type_file()
  + Argument ("template", "the template image").type_image_in()
  + Argument ("output",   "the output image").type_text();

  OPTIONS
  + Option ("first", "indicates that the mesh file is provided by FSL FIRST, so the vertex locations need to be transformed accordingly")
    + Argument ("source_image").type_image_in();


};



void run ()
{

  // Read in the mesh data
  Mesh::Mesh mesh (argument[0]);

  // If the .vtk files come from FIRST, they are defined in a native FSL space, and
  //   therefore the source image must be known in order to transform to real space
  Options opt = get_options ("first");
  if (opt.size()) {
    Image::Header H_first (opt[0][0]);
    mesh.transform_first_to_realspace (H_first);
  }

  // Get the template image
  Image::Header template_image (argument[1]);

  // Create the output image
  mesh.output_pve_image (template_image, argument[2]);

}
