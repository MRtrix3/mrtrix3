/* Copyright (c) 2008-2017 the MRtrix3 contributors.
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


#include "command.h"
#include "image.h"
#include "filter/dwi_brain_mask.h"
#include "filter/mask_clean.h"

using namespace MR;
using namespace App;

#define DEFAULT_CLEAN_SCALE 2

void usage () {
  AUTHOR = "David Raffelt (david.raffelt@florey.edu.au), Thijs Dhollander (thijs.dhollander@gmail.com) and Ben Jeurissen (ben.jeurissen@uantwerpen.be)";

SYNOPSIS = "Generates a whole brain mask from a DWI image";

DESCRIPTION
  + "All diffusion weighted and b=0 volumes are used to "
    "obtain a mask that includes both brain tissue and CSF."

  + "In a second step peninsula-like extensions, where the "
    "peninsula itself is wider than the bridge connecting it "
    "to the mask, are removed. This may help removing "
    "artefacts and non-brain parts, e.g. eyes, from "
    "the mask.";

REFERENCES
  + "Dhollander T, Raffelt D, Connelly A. " // Internal
    "Unsupervised 3-tissue response function estimation from single-shell or multi-shell diffusion MR data without a co-registered T1 image. "
    "ISMRM Workshop on Breaking the Barriers of Diffusion MRI, 2016, 5.";

ARGUMENTS
   + Argument ("input",
    "the input DWI image containing volumes that are both diffusion weighted and b=0")
    .type_image_in ()

   + Argument ("output",
    "the output whole-brain mask image")
    .type_image_out ();

OPTIONS

   + Option ("clean_scale", "the maximum scale used to cut bridges. A certain maximum scale cuts "
                            "bridges up to a width (in voxels) of 2x the provided scale. Setting "
                            "this to 0 disables the mask cleaning step. (Default: " + str(DEFAULT_CLEAN_SCALE, 2) + ")")
    + Argument ("value").type_integer (0, 1e6)

   + DWI::GradImportOptions();
}


void run () {

  auto input = Image<float>::open (argument[0]).with_direct_io (3);
  auto grad = DWI::get_valid_DW_scheme (input);

  if (input.ndim() != 4)
    throw Exception ("input DWI image must be 4D");

  Filter::DWIBrainMask dwi_brain_mask_filter (input, grad);
  dwi_brain_mask_filter.set_message ("computing dwi brain mask");
  auto temp_mask = Image<bool>::scratch (dwi_brain_mask_filter, "brain mask");
  dwi_brain_mask_filter (input, temp_mask);

  Header H_out (temp_mask);
  DWI::stash_DW_scheme (H_out, grad);
  PhaseEncoding::clear_scheme (H_out);
  auto output = Image<bool>::create (argument[1], H_out);

  unsigned int scale = get_option_value ("clean_scale", DEFAULT_CLEAN_SCALE);

  if (scale > 0) {
    try {
      Filter::MaskClean clean_filter (temp_mask, std::string("applying mask cleaning filter"));
      clean_filter.set_scale (scale);
      clean_filter (temp_mask, output);
    } catch (...) {
      WARN("Unable to run mask cleaning filter (image is not truly 3D); skipping");
      for (auto l = Loop (0,3) (temp_mask, output); l; ++l)
        output.value() = temp_mask.value();
    }
  }
  else {
    for (auto l = Loop (0,3) (temp_mask, output); l; ++l)
      output.value() = temp_mask.value();
  }

}
