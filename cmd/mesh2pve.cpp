/* Copyright (c) 2008-2017 the MRtrix3 contributors
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
#include "__mrtrix_plugin.h"

#include "command.h"

#include "header.h"

#include "surface/mesh.h"
#include "surface/algo/mesh2image.h"





using namespace MR;
using namespace App;



void usage ()
{

  AUTHOR = "Robert E. Smith (robert.smith@florey.edu.au)";

  SYNOPSIS = "Convert a mesh surface to a partial volume estimation image";

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
  Surface::Mesh mesh (argument[0]);

  // Get the template image
  Header template_header = Header::open (argument[1]);

  // Create the output image
  Image<float> output = Image<float>::create (argument[2], template_header);

  // Perform the partial volume estimation
  Surface::Algo::mesh2image (mesh, output);

}
