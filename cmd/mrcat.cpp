/* Copyright (c) 2008-2024 the MRtrix3 contributors.
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

#include "algo/loop.h"
#include "command.h"
#include "dwi/gradient.h"
#include "image.h"
#include "phase_encoding.h"
#include "progressbar.h"

using namespace MR;
using namespace App;

// clang-format off
void usage() {

  AUTHOR = "J-Donald Tournier (jdtournier@gmail.com)"
           " and Robert E. Smith (robert.smith@florey.edu.au)";

  SYNOPSIS = "Concatenate several images into one";

  ARGUMENTS
    + Argument ("inputs", "the input images.").type_image_in().allow_multiple()
    + Argument ("output", "the output image.").type_image_out ();

  EXAMPLES
  + Example ("Concatenate individual 3D volumes into a single 4D image series",
             "mrcat volume*.mif series.mif",
             "The wildcard characters will find all images in the current working directory"
             " with names that begin with \"volume\" and end with \".mif\";"
             " the mrcat command will receive these as a list of input file names,"
             " from which it will produce a 4D image"
             " where the input volumes have been concatenated along axis 3"
             " (the fourth axis; the spatial axes are 0, 1 & 2).");

  OPTIONS
  + Option ("axis",
            "specify axis along which concatenation should be performed."
            " By default, the program will use the last non-singleton, non-spatial axis"
            " of any of the input images;"
            " in other words, axis 3,"
            " or whichever axis (greater than 3) of the input images has size greater than one.")
    + Argument ("index").type_integer (0)

  + DataType::options();
}
// clang-format on

template <typename value_type> void write(std::vector<Header> &in, const size_t axis, Header &header_out) {
  auto image_out = Image<value_type>::create(header_out.name(), header_out);
  size_t axis_offset = 0;

  for (size_t i = 0; i != in.size(); i++) {
    auto image_in = in[i].get_image<value_type>();

    auto copy_func = [&axis, &axis_offset](decltype(image_in) &in, decltype(image_out) &out) {
      out.index(axis) = axis < in.ndim() ? in.index(axis) + axis_offset : axis_offset;
      out.value() = in.value();
    };

    ThreadedLoop(
        "concatenating \"" + image_in.name() + "\"", image_in, 0, std::min<size_t>(image_in.ndim(), image_out.ndim()))
        .run(copy_func, image_in, image_out);
    if (axis < image_in.ndim())
      axis_offset += image_in.size(axis);
    else {
      ++axis_offset;
      image_out.index(axis) = axis_offset;
    }
  }
}

void run() {
  const size_t num_images = argument.size() - 1;
  if (num_images == 1) {
    CONSOLE("Only one input image provided; no concatenation to occur");
    CONSOLE("(output image will be identical to input image)");
  }
  std::vector<Header> headers;
  ssize_t max_axis_nonunity = 0;
  for (size_t i = 0; i != num_images; ++i) {
    Header H = Header::open(argument[i]);
    ssize_t a;
    for (a = ssize_t(H.ndim()) - 1; a >= 0 && H.size(a) <= 1; a--)
      ;
    max_axis_nonunity = std::max(max_axis_nonunity, a);
    headers.push_back(std::move(H));
  }
  const size_t axis = get_option_value("axis", std::max(size_t(3), size_t(std::max(ssize_t(0), max_axis_nonunity))));

  Header header_out = concatenate(headers, axis, true);
  header_out.name() = std::string(argument[num_images]);
  header_out.datatype() = DataType::from_command_line(header_out.datatype());

  if (header_out.intensity_offset() == 0.0 && header_out.intensity_scale() == 1.0 &&
      !header_out.datatype().is_floating_point()) {
    switch (header_out.datatype()() & DataType::Type) {
    case DataType::Bit:
    case DataType::UInt8:
    case DataType::UInt16:
    case DataType::UInt32:
      if (header_out.datatype().is_signed())
        write<int32_t>(headers, axis, header_out);
      else
        write<uint32_t>(headers, axis, header_out);
      break;
    case DataType::UInt64:
      if (header_out.datatype().is_signed())
        write<int64_t>(headers, axis, header_out);
      else
        write<uint64_t>(headers, axis, header_out);
      break;
    }
  } else {
    if (header_out.datatype().is_complex())
      write<cdouble>(headers, axis, header_out);
    else
      write<double>(headers, axis, header_out);
  }
}
