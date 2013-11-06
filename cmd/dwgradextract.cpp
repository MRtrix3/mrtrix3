/*
    Copyright 2008 Brain Research Institute, Melbourne, Australia

    Written by J-Donald Tournier, 27/06/08.

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

#include "app.h"
#include "image/header.h"
#include "dwi/gradient.h"

MRTRIX_APPLICATION

using namespace MR;
using namespace App;

void usage () {

  DESCRIPTION
    + "extract diffusion-weighting information from the header of an image";

  ARGUMENTS
  + Argument ("image",  "the input image.").type_image_in ()
  + Argument ("output", "the output text file containing the gradient information").type_text();

  OPTIONS
  + Option ("fsl", "output the gradient information in FSL (bvecs/bvals) format. \n"
                   "This also performs the appropriate re-orientation for FSL's gradient direction convention.");

}



void run () {

  Image::Header header (argument[0]);

  if (!header.DW_scheme().is_set())
    throw Exception ("no gradient information found within image \"" + header.name() + "\"");

  Options opt = get_options ("fsl");
  if (opt.size())
    DWI::save_bvecs_bvals (header, argument[1]);
  else
    header.DW_scheme().save (argument[1]);

}

