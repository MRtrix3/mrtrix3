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
#include "filter/dwi_brain_mask.h"



using namespace MR;
using namespace App;

void usage () {
  AUTHOR = "David Raffelt (david.raffelt@florey.edu.au)";

DESCRIPTION
  + "Generates an whole brain mask from a DWI image."
    "All diffusion weighted and b=0 volumes are used to "
    "obtain a mask that includes both brain tissue and CSF.";

ARGUMENTS
   + Argument ("image",
    "the input DWI image containing volumes that are both diffusion weighted and b=0")
    .type_image_in ()

   + Argument ("image",
    "the output whole brain mask image")
    .type_image_out ();

OPTIONS
  + DWI::GradImportOptions();
}


void run () {
  auto input = Image<float>::open (argument[0]).with_direct_io (3);
  auto grad = DWI::get_DW_scheme (input);
  Filter::DWIBrainMask dwi_brain_mask_filter (input, grad);
  dwi_brain_mask_filter.set_message ("computing dwi brain mask");
  auto output = Image<bool>::create (argument[1], dwi_brain_mask_filter);
  dwi_brain_mask_filter (input, output);
}
