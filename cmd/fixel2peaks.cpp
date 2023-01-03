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
#include "image.h"
#include "progressbar.h"

#include "adapter/subset.h"
#include "algo/loop.h"

#include "fixel/helpers.h"
#include "fixel/keys.h"
#include "fixel/loop.h"
#include "fixel/types.h"

using namespace MR;
using namespace App;

using Fixel::index_type;


void usage ()
{

  AUTHOR = "Robert E. Smith (robert.smith@florey.edu.au)";

  SYNOPSIS = "Convert data in the fixel directory format into a 4D image of 3-vectors";

  DESCRIPTION
  + "If a fixel data file is provided as input, then the 3-vectors in the "
    "output image will be scaled based on the data in that file. If the input "
    "is instead the fixel directory, or the index or directions file, then "
    "all output 3-vectors will possess unit norm.";

  ARGUMENTS
  + Argument ("in",  "the input fixel information").type_various ()
  + Argument ("out", "the output peaks image").type_image_out ();

  OPTIONS
  + Option ("number", "maximum number of fixels in each voxel (default: based on input data)")
    + Argument ("value").type_integer(1)
  + Option ("nan", "fill excess peak data with NaNs rather than zeroes");

}


void run ()
{
  Header index_header, directions_header, data_header;
  try {
    Header input_header = Header::open (argument[0]);
    if (Fixel::is_index_image (input_header)) {
      index_header = std::move (input_header);
      directions_header = Fixel::find_directions_header (Path::dirname (argument[0]));
    } else if (Fixel::is_directions_file (input_header)) {
      index_header = Fixel::find_index_header (Path::dirname (argument[0]));
      directions_header = std::move (input_header);
    } else if (Fixel::is_data_file (input_header)) {
      index_header = Fixel::find_index_header (Path::dirname (argument[0]));
      directions_header = Fixel::find_directions_header (Path::dirname (argument[0]));
      data_header = std::move (input_header);
      Fixel::check_fixel_size (index_header, data_header);
    } else {
      throw Exception ("Input image not recognised as part of fixel format");
    }
  } catch (Exception& e_asimage) {
    try {
      if (!Path::is_dir (argument[0]))
        throw Exception ("Input path is not a directory");
      index_header = Fixel::find_index_header (argument[0]);
      directions_header = Fixel::find_directions_header (argument[0]);
    } catch (Exception& e_asdir) {
      Exception e ("Could not locate fixel data based on input string \"" + argument[0] + "\"");
      e.push_back ("Error when interpreting as image: ");
      for (size_t i = 0; i != e_asimage.num(); ++i)
        e.push_back ("  " + e_asimage[i]);
      e.push_back ("Error when interpreting as fixel directory: ");
      for (size_t i = 0; i != e_asdir.num(); ++i)
        e.push_back ("  " + e_asdir[i]);
      throw e;
    }
  }

  Image<index_type> index_image (index_header.get_image<index_type>());
  Image<float> directions_image (directions_header.get_image<float>());
  Image<float> data_image;
  if (data_header.valid())
    data_image = data_header.get_image<float>();

  auto opt = get_options ("number");
  index_type max_fixel_count = 0;
  if (opt.size()) {
    max_fixel_count = opt[0][0];
  } else {
    for (auto l = Loop (index_image, 0, 3) (index_image); l; ++l)
      max_fixel_count = std::max (max_fixel_count, index_type (index_image.value()));
    INFO ("Maximum number of fixels in any given voxel: " + str(max_fixel_count));
  }

  Header out_header (index_header);
  out_header.datatype() = DataType::Float32;
  out_header.datatype().set_byte_order_native();
  out_header.size(3) = 3 * max_fixel_count;
  out_header.name() = std::string (argument[1]);
  Image<float> out_image (Image<float>::create (argument[1], out_header));

  const float fill = get_options ("nan").size() ? NaN : 0.0f;

  if (data_image.valid()) {
    for (auto l = Loop ("converting fixel data file to peaks image", index_image, 0, 3) (index_image, out_image); l; ++l) {
      out_image.index (3) = 0;
      for (auto f = Fixel::Loop (index_image) (directions_image, data_image); f && out_image.index(3) < out_image.size(3); ++f) {
        for (size_t axis = 0; axis != 3; ++axis) {
          directions_image.index(1) = axis;
          out_image.value() = data_image.value() * directions_image.value();
          ++out_image.index(3);
        }
      }
      for (; out_image.index(3) != out_image.size (3); ++out_image.index(3))
        out_image.value() = fill;
    }
  } else {
    for (auto l = Loop ("converting fixels to peaks image", index_image, 0, 3) (index_image, out_image); l; ++l) {
      out_image.index (3) = 0;
      for (auto f = Fixel::Loop (index_image) (directions_image); f && out_image.index(3) < out_image.size(3); ++f) {
        for (directions_image.index(1) = 0; directions_image.index(1) != 3; ++directions_image.index(1)) {
          out_image.value() = directions_image.value();
          ++out_image.index(3);
        }
      }
      for (; out_image.index(3) != out_image.size (3); ++out_image.index(3))
        out_image.value() = fill;
    }
  }
}

