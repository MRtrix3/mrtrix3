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

#include <vector>

#include "adapter/jacobian.h"
#include "algo/loop.h"
#include "command.h"
#include "file/utils.h"
#include "fixel/fixel.h"
#include "fixel/helpers.h"
#include "fixel/loop.h"
#include "image.h"
#include "interp/nearest.h"
#include "progressbar.h"
#include "registration/warp/helpers.h"

using namespace MR;
using namespace App;

using Fixel::index_type;

// clang-format off
void usage() {
  AUTHOR = "Robert E. Smith (robert.smith@florey.edu.au)";

  SYNOPSIS = "Transform a fixel dataset";

  DESCRIPTION
  + "Unlike the fixelreorient command,"
    " which does not move fixels in space but just reorients them in place"
    " based on the premise of a prior transformation having been applied,"
    " this command additionally involves applying a spatial transformation to input fixel data."

  + "Because it is not trivial to interpolate fixel data at sub-voxel locations,"
    " the resampling following transformation is performed using nearest-neighbour interpolation."
    " This also means that there may be some fixels in the input dataset"
    " for which there is no corresponding fixel created in the output dataset,"
    " as well as fixels in the input dataset for which there are multiple"
    " corresponding fixels created in the output dataset."
    " Finally, there is no assurance of any form of fixel correspondence"
    " between the input and output datasets."

  + "The output fixel dataset will consist of the compulsory index and directions images,"
    " and resampled versions of any fixel data files found in the input directory."
    " Any voxel images present in the input fixel directory will be skipped."
    " Fixel data files with more than one column are currently not supported.";

  ARGUMENTS
  + Argument("fixel_in", "the input fixel directory").type_directory_in()
  + Argument("warp", "the 4D deformation field").type_image_in()
  + Argument("fixel_out", "the output fixel directory").type_directory_out();
}
// clang-format on

using dir_type = Eigen::Vector3f;

void run() {
  const std::string input_fixel_directory = argument[0];
  Fixel::check_fixel_directory(input_fixel_directory);
  Header input_index_header(Fixel::find_index_header(input_fixel_directory));
  Interp::Nearest<Image<index_type>> input_index_image(input_index_header.get_image<index_type>());
  const index_type nfixels_in = Fixel::get_number_of_fixels(input_index_image);
  Header input_directions_header(Fixel::find_directions_header(input_fixel_directory));
  auto input_directions_image = input_directions_header.get_image<float>();

  // Find fixel data files to be transformed
  // Note that any voxel images will need to be handled separately
  auto fixel_data_headers = Fixel::find_data_headers(input_fixel_directory, input_index_header, false);
  // TODO Would be preferable to capture images using their native data type
  std::vector<Image<float>> fixel_data_images;
  for (auto &H : fixel_data_headers) {
    if (H.size(1) > 1)
      throw Exception("Fixel data file \"" + H.name() + "\""                          //
                      + " has more than one column;"                                  //
                      + " fixeltransform command not yet compatible with such data"); //
    fixel_data_images.emplace_back(H.get_image<float>());
  }
  INFO(str(fixel_data_headers.size()) + " fixel data files to be transformed");

  Header warp_header = Header::open(argument[1]);
  Registration::Warp::check_warp(warp_header);
  auto warp_image = warp_header.get_image<float>();
  Adapter::Jacobian<Image<float>> jacobian_adapter(warp_image);

  const std::string output_fixel_directory = argument[2];

  // First pass through data:
  // - Discover how many fixels there will be in the output dataset
  // - Generate statistics on mapping from input to output fixels
  // - Calculate the number of fixels to appear in each output image voxel
  // - Calculate rotated fixel directions ready to populate the output directions image
  Header count_header(warp_header);
  count_header.ndim() = 3;
  Image<index_type> count_buffer = Image<index_type>::scratch(count_header, "scratch fixel count buffer");
  Image<index_type> offset_buffer = Image<index_type>::scratch(count_header, "scratch fixel offset buffer");

  std::vector<dir_type> rotated_directions;
  std::vector<uint8_t> usage_counts(nfixels_in, 0);
  std::vector<std::vector<float>> transformed_fixel_data(fixel_data_images.size());

  for (auto l_voxel = Loop("Computing fixel transformations",
                           count_buffer)(warp_image, jacobian_adapter, count_buffer, offset_buffer);
       l_voxel;
       ++l_voxel) {
    if (input_index_image.scanner(Eigen::Vector3f(warp_image.row(3)))) {
      input_index_image.index(3) = 0;
      const index_type count = input_index_image.value();
      if (count > 0) {
        count_buffer.value() = count;
        offset_buffer.value() = rotated_directions.size();
        const Eigen::Matrix<float, 3, 3> transform = jacobian_adapter.value().inverse();
        for (auto l_fixel = Fixel::Loop(input_index_image)(input_directions_image); l_fixel; ++l_fixel) {
          rotated_directions.push_back((transform * Eigen::Vector3f(input_directions_image.row(1))).normalized());
          usage_counts[input_directions_image.index(0)]++;
          for (size_t fixel_datafile_index = 0; fixel_datafile_index != fixel_data_images.size();
               ++fixel_datafile_index) {
            fixel_data_images[fixel_datafile_index].index(0) = input_directions_image.index(0);
            transformed_fixel_data[fixel_datafile_index].push_back(fixel_data_images[fixel_datafile_index].value());
          }
        }
      }
    }
  }
  const index_type nfixels_out = rotated_directions.size();
  INFO("Number of input vs. output fixels: " + str(nfixels_in) + " -> " + str(nfixels_out));

  // Collect statistics on frequency of input fixels mapping to output fixels
  std::vector<index_type> usage_frequencies;
  for (const auto i : usage_counts) {
    while (i >= usage_frequencies.size())
      usage_frequencies.push_back(0);
    usage_frequencies[i]++;
  }
  INFO("Frequency distribution of utilisation of input fixels:");
  for (index_type count = 0; count != usage_frequencies.size(); ++count) {
    INFO("  " + str(count) + ": " + str(usage_frequencies[count]));
  }

  // Ready to construct output images
  Header output_index_header(warp_header);
  output_index_header.size(3) = 2;
  output_index_header.keyval()[Fixel::n_fixels_key] = str(nfixels_out);
  File::mkdir(output_fixel_directory);
  Image<index_type> output_index_image =                                         //
      Image<index_type>::create(Path::join(output_fixel_directory, "index.mif"), //
                                output_index_header);                            //
  Header output_directions_header(input_directions_header);
  output_directions_header.size(0) = nfixels_out;
  Image<float> output_directions_image =                                         //
      Image<float>::create(Path::join(output_fixel_directory, "directions.mif"), //
                           output_directions_header);                            //

  ProgressBar progress("Writing output fixel data", 2 + fixel_data_images.size());
  for (auto l = Loop(count_header)(count_buffer, offset_buffer, output_index_image); l; ++l) {
    output_index_image.index(3) = 0;
    output_index_image.value() = count_buffer.value();
    output_index_image.index(3) = 1;
    output_index_image.value() = offset_buffer.value();
  }
  ++progress;
  for (index_type i = 0; i != nfixels_out; ++i) {
    output_directions_image.index(0) = i;
    output_directions_image.row(1) = rotated_directions[i];
  }
  ++progress;

  for (size_t fixel_datafile_index = 0; fixel_datafile_index != fixel_data_images.size(); ++fixel_datafile_index) {
    Header H(fixel_data_headers[fixel_datafile_index]);
    H.size(0) = nfixels_out;
    Image<float> output_datafile_image = Image<float>::create(
        Path::join(output_fixel_directory, Path::basename(fixel_data_headers[fixel_datafile_index].name())), H);
    const auto &data = transformed_fixel_data[fixel_datafile_index];
    for (auto l = Loop(0)(output_datafile_image); l; ++l)
      output_datafile_image.value() = data[output_datafile_image.index(0)];
    ++progress;
  }
}
