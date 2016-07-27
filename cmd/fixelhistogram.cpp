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
#include "algo/histogram.h"
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
  AUTHOR = "Robert E. Smith (robert.smith@florey.edu.au)";

  DESCRIPTION
  + "Generate a histogram of fixel values.";

  ARGUMENTS
  + Argument ("input", "the input fixel image.").type_image_in ();

  OPTIONS
  + Algo::Histogram::Options;
}


void run ()
{
  Sparse::Image<FixelMetric> input (argument[0]);

  auto opt = get_options("mask");
  std::unique_ptr<Sparse::Image<FixelMetric> > mask_ptr;
  if (opt.size()) {
    mask_ptr.reset (new Sparse::Image<FixelMetric> (opt[0][0]));
    check_dimensions (input, *mask_ptr);
  }

  File::OFStream output (argument[1]);

  size_t nbins = get_option_value ("bins", 0);
  Algo::Histogram::Calibrator calibrator (nbins, get_options ("ignorezero").size());

  opt = get_options ("template");
  if (opt.size()) {
    calibrator.from_file (opt[0][0]);
  } else {
    for (auto i = Loop (input) (input); i; ++i) {
      if (mask_ptr) {
        assign_pos_of (input).to (*mask_ptr);
        if (input.value().size() != mask_ptr->value().size())
          throw Exception ("the input fixel image and mask image to not have corrresponding fixels");
      }
      for (size_t fixel = 0; fixel != input.value().size(); ++fixel) {
        if (mask_ptr) {
          if (mask_ptr->value()[fixel].value > 0.5)
            calibrator (input.value()[fixel].value);
        } else {
          calibrator (input.value()[fixel].value);
        }
      }
    }
    calibrator.finalize (1, false);
  }

  Algo::Histogram::Data histogram (calibrator);

  for (auto i = Loop (input) (input); i; ++i) {
    if (mask_ptr)
      assign_pos_of (input).to (*mask_ptr);

    for (size_t fixel = 0; fixel != input.value().size(); ++fixel) {
      if (mask_ptr) {
        if (mask_ptr->value()[fixel].value > 0.5)
          histogram (input.value()[fixel].value);
      } else {
        histogram (input.value()[fixel].value);
      }
    }
  }

  for (size_t i = 0; i != nbins; ++i)
    output << (calibrator.get_min() + ((i+0.5) * calibrator.get_bin_width())) << ",";
  output << "\n";
  for (size_t i = 0; i != nbins; ++i)
    output << histogram[i] << ",";
  output << "\n";
}
