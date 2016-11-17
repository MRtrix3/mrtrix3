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

#include "formats/fixel/helpers.h"
#include "formats/fixel/keys.h"
#include "formats/fixel/loop.h"

#include "formats/fixel/legacy/fixel_metric.h"
#include "formats/fixel/legacy/keys.h"
#include "formats/fixel/legacy/image.h"


using namespace MR;
using namespace App;

using Sparse::Legacy::FixelMetric;


void usage ()
{

  AUTHOR = "David Raffelt (david.raffelt@florey.edu.au) and Robert E. Smith (robert.smith@florey.edu.au)";

  DESCRIPTION
  + "convert between the old format fixel image (*.msf / *.msh) and the new fixel directory format";

  ARGUMENTS
  + Argument ("fixel_in",  "the input fixel file / directory.").type_text()
  + Argument ("fixel_out", "the output fixel file / directory.").type_text();

  OPTIONS
  + OptionGroup ("Options for converting from old to new format")
    + Option ("name", "assign a different name to the value field output (Default: value). Do not include the file extension.")
      + Argument ("string").type_text()
    + Option ("nii", "output the index, directions and data file in NIfTI format instead of *.mif")
    + Option ("out_size", "also output the 'size' field from the old format")
    + Option ("template", "specify an existing fixel directory (in the new format) to which the new output should conform")
      + Argument ("path").type_text()

  + OptionGroup ("Options for converting from new to old format")
    + Option ("value", "nominate the data file to import to the 'value' field in the old format")
      + Argument ("path").type_file_in()
    + Option ("in_size", "import data for the 'size' field in the old format")
      + Argument ("path").type_file_in();

}




void convert_old2new ()
{
  Header header (Header::open (argument[0]));
  header.keyval().erase (Sparse::Legacy::name_key);
  header.keyval().erase (Sparse::Legacy::size_key);

  Sparse::Legacy::Image<FixelMetric> input (argument[0]);

  const std::string file_extension = get_options ("nii").size() ? ".nii" : ".mif";

  std::string value_name ("value");
  auto opt = get_options ("name");
  if (opt.size())
    value_name = std::string (opt[0][0]);

  const bool output_size = get_options ("out_size").size();

  const std::string output_fixel_directory = argument[1];
  Sparse::check_fixel_directory (output_fixel_directory, true);

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

  header.keyval()[Sparse::n_fixels_key] = str(fixel_count);
  header.ndim() = 4;
  header.size(3) = 2;
  header.datatype() = DataType::from<uint32_t>();
  header.datatype().set_byte_order_native();

  auto index_image = Image<uint32_t>::create (Path::join (output_fixel_directory, "index" + file_extension), header);
  auto directions_image = Image<float>::create (Path::join (output_fixel_directory, "directions" + file_extension), directions_header).with_direct_io();
  auto value_image = Image<float>::create (Path::join (output_fixel_directory, value_name + file_extension), data_header);
  Image<float> size_image;
  if (output_size)
    size_image = Image<float>::create (Path::join (output_fixel_directory, "size" + file_extension), data_header);

  Image<uint32_t> template_index_image;
  Image<float> template_directions_image;
  opt = get_options ("template");
  if (opt.size()) {
    Sparse::check_fixel_directory (opt[0][0]);
    template_index_image = Sparse::find_index_header (opt[0][0]).get_image<uint32_t>();
    check_dimensions (index_image, template_index_image);
    template_directions_image = Sparse::find_directions_header (opt[0][0]).get_image<float>();
  }

  uint32_t offset = 0;
  for (auto i = Loop ("converting fixel format", input, 0, 3) (input, index_image); i; ++i) {
    const uint32_t num_fixels = input.value().size();
    if (template_index_image.valid()) {
      assign_pos_of (index_image).to (template_index_image);
      template_index_image.index(3) = 0;
      if (template_index_image.value() != num_fixels)
        throw Exception ("mismatch in number of fixels between input and template images");
      template_index_image.index(3) = 1;
      offset = template_index_image.value();
    }
    index_image.index(3) = 0;
    index_image.value() = num_fixels;
    index_image.index(3) = 1;
    index_image.value() = num_fixels ? offset : 0;
    for (size_t f = 0; f != num_fixels; ++f) {
      directions_image.index(0) = offset;
      for (size_t axis = 0; axis != 3; ++axis) {
        directions_image.index(1) = axis;
        directions_image.value() = input.value()[f].dir[axis];
      }
      if (template_directions_image.valid()) {
        template_directions_image.index(0) = offset;
        Eigen::Vector3f template_dir;
        for (size_t axis = 0; axis != 3; ++axis) {
          template_directions_image.index(1) = axis;
          template_dir[axis] = template_directions_image.value();
        }
        if (input.value()[f].dir.dot (template_dir) < 0.999)
          throw Exception ("mismatch in fixel directions between input and template images");
      }
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



void convert_new2old ()
{
  const std::string input_fixel_directory = argument[0];
  auto opt = get_options ("value");
  if (!opt.size())
    throw Exception ("for converting from new to old formats, option -value is compulsory");
  const std::string value_path = get_options ("value")[0][0];
  opt = get_options ("in_size");
  const std::string size_path = opt.size() ? std::string(opt[0][0]) : "";

  Header H_index = Sparse::find_index_header (input_fixel_directory);
  Header H_dirs = Sparse::find_directions_header (input_fixel_directory);
  std::vector<Header> H_data = Sparse::find_data_headers (input_fixel_directory, H_index, false);
  size_t size_index = H_data.size(), value_index = H_data.size();

  for (size_t i = 0; i != H_data.size(); ++i) {
    if (Path::basename (H_data[i].name()) == Path::basename (value_path))
      value_index = i;
    if (Path::basename (H_data[i].name()) == Path::basename (size_path))
      size_index = i;
  }
  if (value_index == H_data.size())
    throw Exception ("could not find image in input fixel directory corresponding to -value option");

  Header H_out (H_index);
  H_out.ndim() = 3;
  H_out.datatype() = DataType::UInt64;
  H_out.datatype().set_byte_order_native();
  H_out.keyval()[Sparse::Legacy::name_key] = str(typeid(FixelMetric).name());
  H_out.keyval()[Sparse::Legacy::size_key] = str(sizeof(FixelMetric));
  Sparse::Legacy::Image<Sparse::Legacy::FixelMetric> out_image (argument[1], H_out);

  auto index_image = H_index.get_image<uint32_t>();
  auto dirs_image = H_dirs.get_image<float>();
  auto value_image = H_data[value_index].get_image<float>();
  Image<float> size_image;
  if (size_index != H_data.size())
    size_image = H_data[size_index].get_image<float>();

  for (auto l = Loop (out_image) (out_image, index_image); l; ++l) {
    index_image.index(3) = 0;
    const uint32_t num_fixels = index_image.value();
    out_image.value().set_size (num_fixels);
    for (auto f = Sparse::FixelLoop (index_image) (dirs_image, value_image); f; ++f) {
      // Construct the direction
      Eigen::Vector3f dir;
      for (size_t axis = 0; axis != 3; ++axis) {
        dirs_image.index(1) = axis;
        dir[axis] = dirs_image.value();
      }
      Sparse::Legacy::FixelMetric fixel (dir, value_image.value(), value_image.value());
      if (size_image.valid()) {
        assign_pos_of (value_image).to (size_image);
        fixel.size = size_image.value();
      }
      out_image.value()[f.fixel_index] = fixel;
    }
  }
}



bool is_old_format (const std::string& path) {
  return (Path::has_suffix (path, ".msf") || Path::has_suffix (path, ".msh"));
}



void run ()
{
  // Detect in which direction the conversion is occurring
  if (is_old_format (argument[0])) {
    if (is_old_format (argument[1]))
      throw Exception ("fixelconvert can only be used to convert between old and new fixel formats; NOT to convert images within the old format");
    convert_old2new ();
  } else {
    if (!is_old_format (argument[1]))
      throw Exception ("fixelconvert can only be used to convert between old and new fixel formats; NOT to convert within the new format");
    convert_new2old ();
  }
}





