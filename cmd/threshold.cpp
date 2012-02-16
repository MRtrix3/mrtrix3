/*
    Copyright 2008 Brain Research Institute, Melbourne, Australia

    Written by J-Donald Tournier, 27/11/09.

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

#include <map>

#include "app.h"
#include "progressbar.h"
#include "image/buffer.h"
#include "image/voxel.h"
#include "image/loop.h"
#include "image/histogram.h"

MRTRIX_APPLICATION

using namespace MR;
using namespace App;

void usage ()
{
  DESCRIPTION
  + "create bitwise image by thresholding image intensity."
  + "By default, the threshold level is determined using a "
  "histogram analysis to cut out the background. Otherwise, "
  "the threshold intensity can be specified using command "
  "line options. Note that only the first study is used for "
  "thresholding.";

  ARGUMENTS
  + Argument ("input", "the input image to be thresholded.").type_image_in ()
  + Argument ("output", "the output binary image mask.").type_image_out ();


  OPTIONS
  + Option ("abs", "specify threshold value as absolute intensity.")
  + Argument ("value").type_float()

  + Option ("percentile", "threshold the image at the ith percentile.")
  + Argument ("value").type_float (0.0, 95.0, 100.0)

  + Option ("top", "provide a mask of the N top-valued voxels")
  + Argument ("N").type_integer (0, 100, std::numeric_limits<int>::max())

  + Option ("bottom", "provide a mask of the N bottom-valued voxels")
  + Argument ("N").type_integer (0, 100, std::numeric_limits<int>::max())

  + Option ("invert", "invert output binary mask.")

  + Option ("toppercent", "provide a mask of the N%% top-valued voxels")
  + Argument ("N").type_integer (0, 100, std::numeric_limits<int>::max())

  + Option ("bottompercent", "provide a mask of the N%% bottom-valued voxels")
  + Argument ("N").type_integer (0, 100, std::numeric_limits<int>::max())

  + Option ("nan", "replace all zero values with NaN.")

  + Option ("ignorezero", "ignore zero-values input voxels.");
}


void run ()
{
  float val (NAN), percentile (NAN), bottomNpercent (NAN), topNpercent (NAN);
  size_t topN (0), bottomN (0), nopt (0);

  Options opt = get_options ("abs");
  if (opt.size()) {
    val = opt[0][0];
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
  bool use_NaN = get_options ("nan").size();
  bool ignore_zeroes = get_options ("ignorezero").size();

  Image::Buffer<float> data_in (argument[0]);
  assert (!data_in.datatype().is_complex());

  if (Image::voxel_count (data_in) < topN || Image::voxel_count (data_in) < bottomN)
    throw Exception ("number of voxels at which to threshold exceeds number of voxels in image");

  if (finite (percentile)) {
    percentile /= 100.0;
    if (percentile < 0.5) {
      bottomN = Math::round (Image::voxel_count (data_in) * percentile);
      invert = !invert;
    }
    else topN = Math::round (Image::voxel_count (data_in) * (1.0 - percentile));
  }

  Image::Header header_out (data_in);
  header_out.datatype() = use_NaN ? DataType::Float32 : DataType::Bit;

  Image::Buffer<float>::voxel_type in (data_in);

  Image::Buffer<float> data_out (header_out, argument[1]);
  Image::Buffer<float>::voxel_type out (data_out);

  float zero = use_NaN ? NAN : 0.0;
  float one  = 1.0;
  if (invert) std::swap (zero, one);

  if (finite (topNpercent) || finite (bottomNpercent)) {
    Image::LoopInOrder loop (in, "computing voxel count...");
    size_t count = 0;
    for (loop.start (in); loop.ok(); loop.next (in)) {
      float val = in.value();
      if (ignore_zeroes && val == 0.0) continue;
      ++count;
    }

    if (finite (topNpercent))
      topN = Math::round (0.01 * topNpercent * count);
    else
      bottomN = Math::round (0.01 * bottomNpercent * count);
  }



  if (topN || bottomN) {
    std::multimap<float,std::vector<ssize_t> > list;

    {
      Image::Loop loop ("thresholding \"" + shorten (in.name()) + "\" at " + (
                            isnan (percentile) ?
                            (str (topN ? topN : bottomN) + "th " + (topN ? "top" : "bottom") + " voxel") :
                              (str (percentile*100.0) + "\% percentile")
                            ) + "...");

      if (topN) {
        for (loop.start (in); loop.ok(); loop.next (in)) {
          const float val = in.value();
          if (ignore_zeroes && val == 0.0) continue;
          if (list.size() == topN) {
            if (val < list.begin()->first) continue;
            list.erase (list.begin());
          }
          std::vector<ssize_t> pos (in.ndim());
          for (size_t n = 0; n < in.ndim(); ++n) pos[n] = in[n];
          list.insert (std::pair<float,std::vector<ssize_t> > (val, pos));
        }
      }
      else {
        for (loop.start (in); loop.ok(); loop.next (in)) {
          const float val = in.value();
          if (ignore_zeroes && val == 0.0) continue;
          if (list.size() == bottomN) {
            std::multimap<float,std::vector<ssize_t> >::iterator i = list.end();
            --i;
            if (val > i->first) continue;
            list.erase (i);
          }
          std::vector<ssize_t> pos (in.ndim());
          for (size_t n = 0; n < in.ndim(); ++n) pos[n] = in[n];
          list.insert (std::pair<float,std::vector<ssize_t> > (val, pos));
        }
      }
    }

    Image::Loop loop;
    for (loop.start (out); loop.ok(); loop.next (out)) {
      out.value() = zero;
    }

    for (std::multimap<float,std::vector<ssize_t> >::const_iterator i = list.begin(); i != list.end(); ++i) {
      for (size_t n = 0; n < out.ndim(); ++n)
        out[n] = i->second[n];
      out.value() = one;
    }
  }
  else {
    if (isnan (val)) {
      Image::Histogram<Image::Buffer<float>::voxel_type> hist (in);
      val = hist.first_min();
    }

    Image::Loop loop ("thresholding \"" + shorten (in.name()) + "\" at intensity " + str (val) + "...");
    for (loop.start (out, in); loop.ok(); loop.next (out, in))
      out.value() = in.value() < val ? zero : one;
  }
}
