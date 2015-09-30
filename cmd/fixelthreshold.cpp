/*
    Copyright 2014 Brain Research Institute, Melbourne, Australia

    Written by David Raffelt, 2014

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
#include "progressbar.h"
#include "algo/loop.h"

#include "image.h"

#include "sparse/fixel_metric.h"
#include "sparse/keys.h"
#include "sparse/image.h"

using namespace MR;
using namespace App;

using Sparse::FixelMetric;

void usage ()
{
  AUTHOR = "David Raffelt (david.raffelt@florey.edu.au)";

  DESCRIPTION
  + "Threshold the values in a fixel image";

  ARGUMENTS
  + Argument ("fixel_in", "the input fixel image.").type_image_in ()
  + Argument ("threshold", "the input threshold").type_float()
  + Argument ("fixel_out",   "the output fixel image").type_image_out ();

  OPTIONS
  + Option ("crop", "remove fixels that fall below threshold (instead of assigning their value to zero or one)");

}



void run ()
{
  auto input_header = Header::open (argument[0]);
  Sparse::Image<FixelMetric> input (argument[0]);

  float threshold = argument[1];

  Sparse::Image<FixelMetric> output (argument[2], input_header);

  auto opt = get_options("crop");

  for (auto i = Loop ("thresholding fixel image...", input) (input, output); i; ++i) {
    if (opt.size()) {
        size_t fixel_count = 0;
        for (size_t f = 0; f != input.value().size(); ++f) {
          if (input.value()[f].value > threshold)
            fixel_count++;
        }
        output.value().set_size (fixel_count);
        fixel_count = 0;
        for (size_t f = 0; f != input.value().size(); ++f) {
          if (input.value()[f].value > threshold)
            output.value()[fixel_count++] = input.value()[f];
        }
    } else {
      output.value().set_size (input.value().size());
      for (size_t f = 0; f != input.value().size(); ++f) {
        output.value()[f] = input.value()[f];
        if (input.value()[f].value > threshold)
          output.value()[f].value = 1.0;
        else
          output.value()[f].value = 0.0;
      }
    }
  }

}

