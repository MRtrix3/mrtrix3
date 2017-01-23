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

#include "stats.h"
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
  + "Compute fixel image statistics";

  ARGUMENTS
  + Argument ("input", "the input fixel image.").type_image_in ();

  OPTIONS
  + Stats::Options;
}


void run ()
{
  Sparse::Image<FixelMetric> input (argument[0]);

  std::unique_ptr<File::OFStream> dumpstream, hist_stream, position_stream;

  auto opt = get_options ("histogram");
  if (opt.size())
    hist_stream.reset (new File::OFStream (opt[0][0]));

  int nbins = DEFAULT_HISTOGRAM_BINS;
  opt = get_options ("bins");
  if (opt.size())
    nbins = opt[0][0];
  Stats::CalibrateHistogram calibrate (nbins);

  opt = get_options ("dump");
  if (opt.size())
    dumpstream.reset (new File::OFStream (opt[0][0]));

  opt = get_options ("position");
  if (opt.size())
    position_stream.reset (new File::OFStream (opt[0][0]));

  vector<std::string> fields;
  opt = get_options ("output");
  for (size_t n = 0; n < opt.size(); ++n)
    fields.push_back (opt[n][0]);

  bool header_shown (!App::log_level || fields.size());

  opt = get_options("mask");
  std::unique_ptr<Sparse::Image<FixelMetric> > mask_ptr;
  if (opt.size()) {
    mask_ptr.reset (new Sparse::Image<FixelMetric> (opt[0][0]));
    check_dimensions (input, *mask_ptr);
  }


  if (hist_stream) {
    for (auto i = Loop (input) (input); i; ++i) {
      if (mask_ptr) {
        assign_pos_of (input).to (*mask_ptr);
        if (input.value().size() != mask_ptr->value().size())
          throw Exception ("the input fixel image and mask image to not have corrresponding fixels");
      }
      for (size_t fixel = 0; fixel != input.value().size(); ++fixel) {
        if (mask_ptr) {
          if (mask_ptr->value()[fixel].value > 0.5)
            calibrate (input.value()[fixel].value);
        } else {
          calibrate (input.value()[fixel].value);
        }
      }
    }
    calibrate.init (*hist_stream);
  }

  Stats::Stats stats (false);

  if (dumpstream)
    stats.dump_to (*dumpstream);

  if (hist_stream)
    stats.generate_histogram (calibrate);

  for (auto i = Loop (input) (input); i; ++i) {
    if (mask_ptr) {
      assign_pos_of (input).to (*mask_ptr);
      if (input.value().size() != mask_ptr->value().size())
        throw Exception ("the input fixel image and mask image to not have corrresponding fixels");
    }

    for (size_t fixel = 0; fixel != input.value().size(); ++fixel) {
      if (mask_ptr) {
        if (mask_ptr->value()[fixel].value > 0.5)
          stats (input.value()[fixel].value);
      } else {
        stats (input.value()[fixel].value);
      }
    }
  }

  if (!header_shown)
    Stats::print_header (false);
  header_shown = true;

  stats.print (input, fields);

  if (hist_stream)
    stats.write_histogram (*hist_stream);

}
