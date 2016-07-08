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


#include <map>
#include <vector>

#include "command.h"
#include "image.h"
#include "image_helpers.h"
#include "memory.h"
#include "progressbar.h"

#include "algo/loop.h"
#include "algo/histogram.h"
#include "filter/optimal_threshold.h"


using namespace MR;
using namespace App;

void usage ()
{
  AUTHOR = "J-Donald Tournier (jdtournier@gmail.com)";

  DESCRIPTION
  + "create bitwise image by thresholding image intensity. By default, an "
    "optimal threshold is determined using a parameter-free method. "
    "Alternatively the threshold can be defined manually by the user "
    "or using a histogram-based analysis to cut out the background.";

  REFERENCES 
    + "* If not using the -abs option:\n"
    "Ridgway, G. R.; Omar, R.; Ourselin, S.; Hill, D. L.; Warren, J. D. & Fox, N. C. "
    "Issues with threshold masking in voxel-based morphometry of atrophied brains. "
    "NeuroImage, 2009, 44, 99-111";

  ARGUMENTS
  + Argument ("input", "the input image to be thresholded.").type_image_in ()
  + Argument ("output", "the output binary image mask.").type_image_out ();


  OPTIONS
  + Option ("abs", "specify threshold value as absolute intensity.")
  + Argument ("value").type_float()

  + Option ("histogram", "define the threshold by a histogram analysis to cut out the background. "
                         "Note that only the first study is used for thresholding.")

  + Option ("percentile", "threshold the image at the ith percentile.")
  + Argument ("value").type_float (0.0, 100.0)

  + Option ("top", "provide a mask of the N top-valued voxels")
  + Argument ("N").type_integer (0)

  + Option ("bottom", "provide a mask of the N bottom-valued voxels")
  + Argument ("N").type_integer (0)

  + Option ("invert", "invert output binary mask.")

  + Option ("toppercent", "provide a mask of the N%% top-valued voxels")
  + Argument ("N").type_float (0.0, 100.0)

  + Option ("bottompercent", "provide a mask of the N%% bottom-valued voxels")
  + Argument ("N").type_float (0.0, 100.0)

  + Option ("nan", "use NaN as the output zero value.")

  + Option ("ignorezero", "ignore zero-valued input voxels.")

  + Option ("mask", "compute the optimal threshold based on voxels within a mask.")
  + Argument ("image").type_image_in ();
}


void run ()
{
  default_type threshold_value (NaN), percentile (NaN), bottomNpercent (NaN), topNpercent (NaN);
  bool use_histogram = false;
  size_t topN (0), bottomN (0), nopt (0);

  auto opt = get_options ("abs");
  if (opt.size()) {
    threshold_value = opt[0][0];
    ++nopt;
  }

  opt = get_options ("histogram");
  if (opt.size()) {
    use_histogram = true;
    ++nopt;
  }

  opt = get_options ("percentile");
  if (opt.size()) {
    percentile = opt[0][0];
    ++nopt;
  }

  opt = get_options ("top");
  if (opt.size()) {
    topN = opt[0][0];
    ++nopt;
  }

  opt = get_options ("bottom");
  if (opt.size()) {
    bottomN = opt[0][0];
    ++nopt;
  }

  opt = get_options ("toppercent");
  if (opt.size()) {
    topNpercent = opt[0][0];
    ++nopt;
  }

  opt = get_options ("bottompercent");
  if (opt.size()) {
    bottomNpercent = opt[0][0];
    ++nopt;
  }

  if (nopt > 1)
    throw Exception ("too many conflicting options");

  bool invert = get_options ("invert").size();
  const bool use_NaN = get_options ("nan").size();
  const bool ignore_zeroes = get_options ("ignorezero").size();

  auto header = Header::open (argument[0]);
  if (header.datatype().is_complex())
    throw Exception ("Cannot perform thresholding on complex images");
  auto in = header.get_image<float>();

  if (voxel_count (in) < topN || voxel_count (in) < bottomN)
    throw Exception ("number of voxels at which to threshold exceeds number of voxels in image");

  if (std::isfinite (percentile)) {
    percentile /= 100.0;
    if (percentile < 0.5) {
      bottomN = std::round (voxel_count (in) * percentile);
      invert = !invert;
    }
    else topN = std::round (voxel_count (in) * (1.0 - percentile));
  }

  header.datatype() = use_NaN ? DataType::Float32 : DataType::Bit;

  auto out = Image<float>::create (argument[1], header);

  float zero = use_NaN ? NaN : 0.0;
  float one  = 1.0;
  if (invert) std::swap (zero, one);

  if (std::isfinite (topNpercent) || std::isfinite (bottomNpercent)) {
    size_t count = 0;
    for (auto l = Loop("computing voxel count", in) (in); l; ++l) {
      if (ignore_zeroes && in.value() == 0.0) continue;
      ++count;
    }
    if (std::isfinite (topNpercent))
      topN = std::round (0.01 * topNpercent * count);
    else
      bottomN = std::round (0.01 * bottomNpercent * count);
  }

  if (topN || bottomN) {
    std::multimap<float,std::vector<ssize_t> > list;

    {
      const std::string msg = "thresholding \"" + shorten (in.name()) + "\" at " + (
                              std::isnan (percentile) ?
                              (str (topN ? topN : bottomN) + "th " + (topN ? "top" : "bottom") + " voxel") :
                              (str (percentile*100.0) + "\% percentile"));

      if (topN) {
        for (auto l = Loop(in) (in); l; ++l) {
          const float val = in.value();
          if (!std::isfinite (val)) continue;
          if (ignore_zeroes && val == 0.0) continue;
          if (list.size() == topN) {
            if (val < list.begin()->first) continue;
            list.erase (list.begin());
          }
          std::vector<ssize_t> pos (in.ndim());
          for (size_t n = 0; n < in.ndim(); ++n)
            pos[n] = in.index(n);
          list.insert (std::pair<float,std::vector<ssize_t> > (val, pos));
        }
      }
      else {
        for (auto l = Loop(in) (in); l; ++l) {
          const float val = in.value();
          if (!std::isfinite (val)) continue;
          if (ignore_zeroes && val == 0.0) continue;
          if (list.size() == bottomN) {
            std::multimap<float,std::vector<ssize_t> >::iterator i = list.end();
            --i;
            if (val > i->first) continue;
            list.erase (i);
          }
          std::vector<ssize_t> pos (in.ndim());
          for (size_t n = 0; n < in.ndim(); ++n)
            pos[n] = in.index(n);
          list.insert (std::pair<float,std::vector<ssize_t> > (val, pos));
        }
      }
    }

    for (auto l = Loop(out) (out); l; ++l)
      out.value() = zero;

    for (std::multimap<float,std::vector<ssize_t> >::const_iterator i = list.begin(); i != list.end(); ++i) {
      for (size_t n = 0; n < out.ndim(); ++n)
        out.index(n) = i->second[n];
      out.value() = one;
    }
  }
  else {
    if (use_histogram) {
      Histogram<decltype(in)> hist (in);
      threshold_value = hist.first_min();
    }
    else if (std::isnan (threshold_value)) {
      Image<bool> mask;
      opt = get_options ("mask");
      if (opt.size())
        mask = Image<bool>::open (opt[0][0]);
      threshold_value = Filter::estimate_optimal_threshold (in, mask);
    }

    const std::string msg = "thresholding \"" + shorten (in.name()) + "\" at intensity " + str (threshold_value);
    for (auto l = Loop(msg, in) (in, out); l; ++l) {
      const float val = in.value();
      out.value() = ( !std::isfinite (val) || val < threshold_value ) ? zero : one;
    }
  }
}
