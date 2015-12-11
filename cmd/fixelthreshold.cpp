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
  + Argument ("fixel_out", "the output fixel image").type_image_out ();

  OPTIONS
  + Option ("crop", "remove fixels that fall below threshold (instead of assigning their value to zero or one)")
  + Option ("invert", "invert the output image (i.e. below threshold fixels are included instead)");

}



void run ()
{
  auto input_header = Header::open (argument[0]);
  Sparse::Image<FixelMetric> input (argument[0]);

  float threshold = argument[1];

  Sparse::Image<FixelMetric> output (argument[2], input_header);

  auto opt = get_options("crop");
  const bool invert = get_options("invert").size();

  for (auto i = Loop ("thresholding fixel image", input) (input, output); i; ++i) {
    if (opt.size()) {
        size_t fixel_count = 0;
        for (size_t f = 0; f != input.value().size(); ++f) {
          if (invert) {
            if (input.value()[f].value < threshold)
              fixel_count++;
          } else {
            if (input.value()[f].value > threshold)
              fixel_count++;
          }
        }
        output.value().set_size (fixel_count);
        fixel_count = 0;
        for (size_t f = 0; f != input.value().size(); ++f) {
          if (invert) {
            if (input.value()[f].value < threshold)
              output.value()[fixel_count++] = input.value()[f];
          } else {
            if (input.value()[f].value > threshold)
              output.value()[fixel_count++] = input.value()[f];
          }
        }
    } else {
      output.value().set_size (input.value().size());
      for (size_t f = 0; f != input.value().size(); ++f) {
        output.value()[f] = input.value()[f];
        if (input.value()[f].value > threshold) {
          output.value()[f].value = (invert) ? 0.0 : 1.0;
        } else {
          output.value()[f].value = (invert) ? 1.0 : 0.0;
        }
      }
    }
  }

}

