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
#include "algo/loop.h"
#include "dwi/gradient.h"
#include "dwi/shells.h"


#define DEFAULT_TARGET_INTENSITY 1000.0


using namespace MR;
using namespace App;

void usage () {
  AUTHOR = "David Raffelt (david.raffelt@florey.edu.au)";

DESCRIPTION
  + "Intensity normalise the b=0 signal within a supplied white matter mask";

ARGUMENTS
   + Argument ("input",
    "the input DWI image containing volumes that are both diffusion weighted and b=0").type_image_in ()

   + Argument ("mask",
    "the input mask image used to normalise the intensity").type_image_in ()

   + Argument ("output",
    "the output DWI intensity normalised image").type_image_out ();

OPTIONS
  + Option ("intensity", "normalise the b=0 signal to the specified value (Default: " + str(DEFAULT_TARGET_INTENSITY, 1) + ")")
  + Argument ("value").type_float()

  + Option ("percentile", "define the percentile of the mask intensties used for normalisation. "
                          "If this option is not supplied then the median value (50th percentile) will be "
                          "normalised to the desired intensity value.")
  + Argument ("value").type_integer (0, 100)

  + DWI::GradImportOptions();

}



void run () {

  auto input = Image<float>::open (argument[0]);
  auto mask = Image<bool>::open (argument[1]);
  check_dimensions (input, mask, 0, 3);


  auto grad = DWI::get_DW_scheme (input);
  DWI::Shells grad_shells (grad);

  vector<size_t> bzeros;
  for (size_t s = 0; s < grad_shells.count(); ++s) {
    if (grad_shells[s].is_bzero()) {
      bzeros = grad_shells[s].get_volumes();
    }
  }

  vector<float> bzero_mask_values;
  float intensity = get_option_value ("intensity", DEFAULT_TARGET_INTENSITY);
  int percentile = get_option_value ("percentile", 50);

  for (auto i = Loop ("computing " + str(percentile) + "th percentile within mask", input, 0, 3) (input, mask); i; ++i) {
    if (mask.value()) {
      float mean_bzero = 0.0;
      for (size_t v = 0; v < bzeros.size(); ++v) {
        input.index(3) = bzeros[v];
        mean_bzero += input.value();
      }
      mean_bzero /= (float)bzeros.size();
      bzero_mask_values.push_back(mean_bzero);
    }
  }

  std::sort (bzero_mask_values.rbegin(), bzero_mask_values.rend());
  float percentile_value = bzero_mask_values[round(float(bzero_mask_values.size()) * float(percentile) / 100.0)];
  float scale_factor = intensity / percentile_value;

  Header output_header (input);
  output_header.keyval()["dwi_norm_scale_factor"] = str(scale_factor);
  output_header.keyval()["dwi_norm_percentile"] = str(percentile);
  auto output = Image<float>::create (argument[2], output_header);

  for (auto i = Loop ("normalising image intensities", input) (input, output); i; ++i)
    output.value() = input.value() * scale_factor;
}

