/* Copyright (c) 2008-2023 the MRtrix3 contributors.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Covered Software is provided under this License on an "as is"
 * basis, without warranty of any kind, either expressed, implied, or
 * statutory, including, without limitation, warranties that the
 * Covered Software is free of defects, merchantable, fit for a
 * particular purpose or non-infringing.
 * See the Mozilla Public License v. 2.0 for more details.
 *
 * For more details, see http://www.mrtrix.org/.
 */

#include "command.h"
#include "datatype.h"
#include "header.h"
#include "image.h"
#include "mrtrix.h"
#include "stride.h"
#include "types.h"

#include "algo/loop.h"
#include "algo/min_max.h"
#include "colourmap.h"

using namespace MR;
using namespace App;


vector<std::string> colourmap_choices_std;
vector<const char*> colourmap_choices_cstr;

void usage ()
{

  const ColourMap::Entry* entry = ColourMap::maps;
  do {
    if (strcmp(entry->name, "Complex"))
      colourmap_choices_std.push_back (lowercase (entry->name));
    ++entry;
  } while (entry->name);
  colourmap_choices_cstr.reserve (colourmap_choices_std.size() + 1);
  for (const auto& s : colourmap_choices_std)
    colourmap_choices_cstr.push_back (s.c_str());
  colourmap_choices_cstr.push_back (nullptr);

  AUTHOR = "Robert E. Smith (robert.smith@florey.edu.au)";

  SYNOPSIS = "Apply a colour map to an image";

  DESCRIPTION
  + "Under typical usage, this command will receive as input ad 3D greyscale image, and "
    "output a 4D image with 3 volumes corresponding to red-green-blue components; "
    "other use cases are possible, and are described in more detail below."

  + "By default, the command will automatically determine the maximum and minimum "
    "intensities of the input image, and use that information to set the upper and "
    "lower bounds of the applied colourmap. This behaviour can be overridden by manually "
    "specifying these bounds using the -upper and -lower options respectively.";

  ARGUMENTS
  + Argument ("input",  "the input image").type_image_in()
  + Argument ("map",    "the colourmap to apply; choices are: " + join(colourmap_choices_std, ",")).type_choice (colourmap_choices_cstr.data())
  + Argument ("output", "the output image").type_image_out();

  OPTIONS
  + Option ("upper", "manually set the upper intensity of the colour mapping")
    + Argument ("value").type_float()

  + Option ("lower", "manually set the lower intensity of the colour mapping")
    + Argument ("value").type_float()

  + Option ("colour", "set the target colour for use of the 'colour' map (three comma-separated floating-point values)")
    + Argument ("values").type_sequence_float();

}





void run ()
{
  Header H_in = Header::open (argument[0]);
  const ColourMap::Entry colourmap = ColourMap::maps[argument[1]];
  Eigen::Vector3d fixed_colour (NaN, NaN, NaN);
  if (colourmap.is_colour) {
    if (!(H_in.ndim() == 3 || (H_in.ndim() == 4 && H_in.size(3) == 1)))
      throw Exception ("For applying a fixed colour, command expects a 3D image as input");
    auto opt = get_options ("colour");
    if (!opt.size())
      throw Exception ("For \'colour\' colourmap, target colour must be specified using the -colour option");
    const auto values = parse_floats (opt[0][0]);
    if (values.size() != 3)
      throw Exception ("Target colour must be specified as a comma-separated list of three values");
    fixed_colour = Eigen::Vector3d (values.data());
    if (fixed_colour.minCoeff() < 0.0)
      throw Exception ("Values for fixed colour provided via -colour option cannot be negative");
  } else if (colourmap.is_rgb) {
    if (!(H_in.ndim() == 4 && H_in.size(3) == 3))
      throw Exception ("\'rgb\' colourmap only applies to 4D images with 3 volumes");
    if (get_options ("lower").size()) {
      WARN ("Option -lower ignored: not compatible with \'rgb\' colourmap (a minimum of 0.0 is assumed)");
    }
  } else {
    if (!(H_in.ndim() == 3 || (H_in.ndim() == 4 && H_in.size(3) == 1)))
      throw Exception ("For standard colour maps, command expects a 3D image as input");
    if (get_options ("colour").size()) {
      WARN ("Option -colour ignored: only applies if \'colour\' colourmap is used");
    }
  }

  auto in = H_in.get_image<float>();
  float lower = colourmap.is_rgb ? 0.0 : get_option_value ("lower", NaN);
  float upper = get_option_value ("upper", NaN);
  if (!std::isfinite (lower) || !std::isfinite (upper)) {
    float image_min = NaN, image_max = NaN;
    min_max (in, image_min, image_max);
    if (colourmap.is_rgb) { // RGB
      image_max = std::max (MR::abs (image_min), MR::abs (image_max));
    } else {
      if (!std::isfinite (lower)) {
        if (!std::isfinite (image_min))
          throw Exception ("Unable to determine minimum value from image");
        lower = image_min;
      }
    }
    if (!std::isfinite (upper)) {
      if (!std::isfinite (image_max))
        throw Exception ("Unable to determine maximum value from image");
      upper = image_max;
    }
  }
  const float multiplier = 1.0f / (upper - lower);

  auto scale = [&] (const float value) { return std::max (0.0f, std::min (1.0f, multiplier * (value - lower))); };

  Header H_out (H_in);
  H_out.ndim() = 4;
  H_out.size(3) = 3;
  Stride::set (H_out, Stride::contiguous_along_axis (3, H_out));
  H_out.datatype() = DataType::Float32;
  H_out.datatype().set_byte_order_native();
  auto out = Image<float>::create (argument[2], H_out);


  if (colourmap.is_colour) {
    assert (fixed_colour.allFinite());
    for (auto l_outer = Loop ("Applying fixed RGB colour to greyscale image", H_in) (in, out); l_outer; ++l_outer) {
      const float amplitude = std::max (0.0f, std::min (1.0f, scale (in.value())));
      for (auto l_inner = Loop(3) (out); l_inner; ++l_inner)
        out.value() = amplitude * fixed_colour[out.index(3)];
    }
  } else if (colourmap.is_rgb) {
    for (auto l_outer = Loop ("Scaling RGB colour image", H_in) (in, out); l_outer; ++l_outer)
      out.value() = scale (in.value());
  } else {
    const ColourMap::Entry::basic_map_fn& map_fn = colourmap.basic_mapping;
    for (auto l_outer = Loop ("Mapping intensities to RGB colours", H_in) (in, out); l_outer; ++l_outer) {
      const Eigen::Array3f colour = map_fn (scale (in.value()));
      for (auto l_inner = Loop(3) (out); l_inner; ++l_inner)
        out.value() = colour[out.index(3)];
    }
  }
}
