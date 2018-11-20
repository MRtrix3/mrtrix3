/*
 * Copyright (c) 2008-2018 the MRtrix3 contributors.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, you can obtain one at http://mozilla.org/MPL/2.0/
 *
 * MRtrix3 is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * For more details, see http://www.mrtrix.org/
 */


#include "command.h"
#include "progressbar.h"
#include "algo/loop.h"

#include "image.h"

#include "fixel/helpers.h"
#include "fixel/keys.h"

using namespace MR;
using namespace App;


void usage ()
{
  AUTHOR = "David Raffelt (david.raffelt@florey.edu.au) & Rami Tabarra (rami.tabarra@florey.edu.au)";

  SYNOPSIS = "Crop/remove fixels from sparse fixel image using a binary fixel mask";

  DESCRIPTION
  + "The mask must be input as a fixel data file the same dimensions as the fixel data file(s) to be cropped.";

  ARGUMENTS
  + Argument ("input_fixel_directory", "input fixel directory, all data files and directions "
                                       "file will be cropped and saved in the output fixel directory").type_directory_in()
  + Argument ("input_fixel_mask", "the input fixel data file defining which fixels to crop. "
                                  "Fixels with zero values will be removed").type_image_in ()
  + Argument ("output_fixel_directory", "the output directory to store the cropped directions and data files").type_directory_out();
}



void run ()
{
  const auto in_directory = argument[0];
  Fixel::check_fixel_directory (in_directory);
  Header in_index_header = Fixel::find_index_header (in_directory);
  auto in_index_image = in_index_header.get_image <uint32_t>();

  auto mask_image = Image<bool>::open (argument[1]);
  Fixel::check_fixel_size (in_index_image, mask_image);

  const auto out_fixel_directory = argument[2];
  Fixel::check_fixel_directory (out_fixel_directory, true);

  Header out_header = Header (in_index_image);
  size_t total_nfixels = Fixel::get_number_of_fixels (in_index_header);

  // We need to do a first pass of the mask image to determine the number of cropped fixels
  for (auto l = Loop (0) (mask_image); l; ++l) {
    if (!mask_image.value())
      total_nfixels --;
  }

  out_header.keyval ()[Fixel::n_fixels_key] = str (total_nfixels);
  auto out_index_image = Image<uint32_t>::create (Path::join (out_fixel_directory, Path::basename (in_index_image.name())), out_header);


  // Open all data images and create output date images with size equal to expected number of fixels
  vector<Header> in_headers = Fixel::find_data_headers (in_directory, in_index_header, true);
  vector<Image<float> > in_data_images;
  vector<Image<float> > out_data_images;
  for (auto& in_data_header : in_headers) {
    in_data_images.push_back(in_data_header.get_image<float>().with_direct_io());
    check_dimensions (in_data_images.back(), mask_image, {0, 2});

    Header out_data_header (in_data_header);
    out_data_header.size (0) = total_nfixels;
    out_data_images.push_back(Image<float>::create (Path::join (out_fixel_directory, Path::basename (in_data_header.name())),
                                                    out_data_header).with_direct_io());
  }

  mask_image.index (1) = 0;
  uint32_t out_offset = 0;
  for (auto l = Loop ("cropping fixel image", 0, 3) (in_index_image, out_index_image); l; ++l) {

    in_index_image.index(3) = 0;
    size_t in_nfixels = in_index_image.value();
    in_index_image.index(3) = 1;
    uint32_t in_offset = in_index_image.value();

    size_t out_nfixels = 0;
    for (size_t i = 0; i < in_nfixels; ++i) {
      mask_image.index(0) = in_offset + i;
      if (mask_image.value()) {
        for (size_t d = 0; d < in_data_images.size(); ++d) {
          out_data_images[d].index(0) = out_offset + out_nfixels;
          in_data_images[d].index(0) = in_offset + i;
          out_data_images[d].row(1) = in_data_images[d].row(1);
        }
        out_nfixels++;
      }
    }

    out_index_image.index(3) = 0;
    out_index_image.value() = out_nfixels;
    out_index_image.index(3) = 1;
    out_index_image.value() = (out_nfixels) ? out_offset : 0;
    out_offset += out_nfixels;
  }

}
