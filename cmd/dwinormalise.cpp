/*
    Copyright 2008 Brain Research Institute, Melbourne, Australia

    Written by David Raffelt, 24/06/11.

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
#include "image.h"
#include "algo/loop.h"
#include "dwi/gradient.h"
#include "dwi/shells.h"

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
  + Option ("intensity", "normalise the b=0 signal to the specified value (Default: 1000)")
  + Argument ("value").type_float()

  + Option ("percentile", "define the percentile of the mask intensties used for normalisation. "
                          "If this option is not supplied then the median value (50th percentile) will be "
                          "normalised to the desired intensity value.")
  + Argument ("value").type_integer (0, 50, 100)

  + DWI::GradImportOptions();

}



void run () {

  auto input = Image<float>::open (argument[0]);
  auto mask = Image<bool>::open (argument[1]);
  auto output = Image<float>::create (argument[2], input);

  check_dimensions (input, mask, 0, 3);

  auto grad = DWI::get_DW_scheme (input);
  DWI::Shells grad_shells (grad);

  std::vector<size_t> bzeros;
  for (size_t s = 0; s < grad_shells.count(); ++s) {
    if (grad_shells[s].is_bzero()) {
      bzeros = grad_shells[s].get_volumes();
    }
  }


  std::vector<float> bzero_mask_values;
  float intensity = get_option_value ("intensity", 1000);
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
  for (auto i = Loop ("normalising image intensities", input) (input, output); i; ++i)
    output.value() = input.value() * scale_factor;
}

