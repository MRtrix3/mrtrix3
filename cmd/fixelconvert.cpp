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
#include "image.h"
#include "progressbar.h"

#include "algo/loop.h"

#include "math/SH.h"

#include "fixel_format/helpers.h"
#include "fixel_format/keys.h"

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
  + "convert an old format fixel image (*.msf) to the new fixel folder format";

  ARGUMENTS
  + Argument ("fixel_in", "the input fixel file.").type_image_in ()
  + Argument ("fixel_output", "the output fixel folder.").type_text();

  OPTIONS
  + Option ("name", "assign a different name to the value field output (Default: value). Do not include the file extension.")
      + Argument ("string").type_text()

  + Option ("nii", "output the index, directions and data file in NifTI format instead of *.mif")

  + Option ("size", "also output the 'size' field from the old format");
}


void run ()
{
  Header header (Header::open (argument[0]));
  header.keyval().erase (Sparse::name_key);
  header.keyval().erase (Sparse::size_key);

  Sparse::Image<FixelMetric> input (argument[0]);

  std::string file_extension (".mif");
  if (get_options ("nii").size())
    file_extension = ".nii";

  std::string value_name ("value");
  auto opt = get_options ("name");
  if (opt.size())
    value_name = std::string (opt[0][0]);

  const bool output_size = get_options ("size").size();

  std::string output_fixel_folder = argument[1];
  FixelFormat::check_fixel_folder (output_fixel_folder, true);

  uint32_t fixel_count = 0;
  for (auto i = Loop (input) (input); i; ++i)
    fixel_count += input.value().size();

  Header data_header (header);
  data_header.ndim() = 3;
  data_header.size(0) = fixel_count;
  data_header.size(1) = 1;
  data_header.size(2) = 1;
  data_header.datatype () = DataType::Float32;
  data_header.datatype ().set_byte_order_native();

  Header directions_header (data_header);
  directions_header.size(1) = 3;

  header.keyval()[FixelFormat::n_fixels_key] = str(fixel_count);
  header.ndim() = 4;
  header.size(3) = 2;
  header.datatype() = DataType::from<uint32_t>();
  header.datatype().set_byte_order_native();

  auto index_image = Image<uint32_t>::create (Path::join (output_fixel_folder, "index" + file_extension), header);
  auto directions_image = Image<float>::create (Path::join (output_fixel_folder, "directions" + file_extension), directions_header).with_direct_io();
  auto value_image = Image<float>::create (Path::join (output_fixel_folder, value_name + file_extension), data_header);
  Image<float> size_image;
  if (output_size)
    size_image = Image<float>::create (Path::join (output_fixel_folder, "size" + file_extension), data_header);

  int32_t offset = 0;
  for (auto i = Loop ("converting fixel format", input, 0, 3) (input, index_image); i; ++i) {
    index_image.index(3) = 0;
    index_image.value() = (int32_t)input.value().size();
    index_image.index(3) = 1;
    index_image.value() = offset;
    for (size_t f = 0; f != input.value().size(); ++f) {
      directions_image.index(0) = offset;
      directions_image.row(1) = input.value()[f].dir;
      value_image.index(0) = offset;
      value_image.value() = input.value()[f].value;
      if (size_image.valid()) {
        size_image.index(0) = offset;
        size_image.value() = input.value()[f].size;
      }
      offset++;
    }
  }
}
