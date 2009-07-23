/*
    Copyright 2008 Brain Research Institute, Melbourne, Australia

    Written by J-Donald Tournier, 27/06/08.

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

#include "app.h"
#include "progressbar.h"
#include "image/voxel.h"
#include "image/misc.h"
#include "histogram.h"
#include "min_max.h"

using namespace std; 
using namespace MR; 

SET_VERSION_DEFAULT;

DESCRIPTION = {
 "create bitwise image by thresholding image intensity.",
 "By default, the threshold level is determined using a histogram analysis to cut out the background. Otherwise, the threshold intensity can be specified using command line options. Note that only the first study is used for thresholding.",
  NULL
};

ARGUMENTS = {
  Argument ("input", "input image", "the input image to be thresholded.").type_image_in (),
  Argument ("output", "output image", "the output binary image mask.").type_image_out (),
  Argument::End
};


OPTIONS = { 
  Option ("abs", "absolute threshold", "specify threshold value as absolute intensity.")
    .append (Argument ("value", "value", "the absolute threshold to use.").type_float (NAN, NAN, 0.0)),

  Option ("percent", "percentage threshold", "specify threshold value as a percentage of the peak intensity in the input image.")
    .append (Argument ("value", "value", "the percentage threshold to use.").type_float (NAN, NAN, 0.0)),

  Option ("invert", "invert mask.", "invert output binary mask."),

  Option ("nan", "use NaN.", "replace all zero values with NaN."),

  Option::End 
};


EXECUTE {

  bool use_percentage = false, invert = false, use_NaN = false, optimise = true;
  float val = NAN;

  std::vector<OptBase> opt = get_options (0);
  if (opt.size()) {
    use_percentage = false;
    optimise = false;
    val = opt[0][0].get_float();
  }

  opt = get_options (1);
  if (opt.size()) {
    use_percentage = true;
    optimise = false;
    val = opt[0][0].get_float();
  }

  if (get_options(2).size()) invert = true;
  if (get_options(3).size()) use_NaN = true;

  Image::Voxel in (*argument[0].get_image());
  Image::Header header (in.image.header());

  if (in.is_complex()) header.data_type = DataType::CFloat32;
  else {
    if (use_NaN) header.data_type = DataType::Float32;
    else header.data_type = DataType::Bit;
  }

  Image::Voxel out (*argument[1].get_image (header));

  if (use_percentage) {
    float min, max;
    get_min_max (in, min, max);
    val = min + 0.01*val*(max-min);
  }

  in.image.map();
  if (optimise) {
    Histogram hist (in);
    val = hist.first_min();
  }

  float zero = use_NaN ? NAN : 0.0;
  float one  = invert ? zero : 1.0;
  zero = invert ? 1.0 : zero;

  out.image.map();
  ProgressBar::init (voxel_count(out), "thresholding at intensity " + str(val) + "...");

  do {
    float v = in.real();
    out.real() = v > val ? one : zero;
    if (out.is_complex()) {
      v = in.imag();
      out.imag() = v > val ? one : zero;
    }

    ProgressBar::inc();
    in++;
  } while (out++);

  ProgressBar::done();
}
