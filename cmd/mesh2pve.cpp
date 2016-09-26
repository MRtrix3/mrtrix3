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

#include "header.h"

#include "mesh/mesh.h"





using namespace MR;
using namespace App;



void usage ()
{

  AUTHOR = "Robert E. Smith (robert.smith@florey.edu.au)";

  DESCRIPTION
  + "convert a mesh surface to a partial volume estimation image.";

  REFERENCES 
    + "Smith, R. E.; Tournier, J.-D.; Calamante, F. & Connelly, A. " // Internal
    "Anatomically-constrained tractography: Improved diffusion MRI streamlines tractography through effective use of anatomical information. "
    "NeuroImage, 2012, 62, 1924-1938";

  ARGUMENTS
  + Argument ("source",   "the mesh file; note vertices must be defined in realspace coordinates").type_file_in()
  + Argument ("template", "the template image").type_image_in()
  + Argument ("output",   "the output image").type_image_out();

}



void run ()
{

  // Read in the mesh data
  Mesh::Mesh mesh (argument[0]);

  // Get the template image
  Header template_image = Header::open (argument[1]);

  // Create the output image
  mesh.output_pve_image (template_image, argument[2]);

}
