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
#include "progressbar.h"
#include "algo/loop.h"

#include "image.h"

#include "fixel_format/helpers.h"
#include "fixel_format/keys.h"

using namespace MR;
using namespace App;


void usage ()
{
  AUTHOR = "David Raffelt (david.raffelt@florey.edu.au)";

  DESCRIPTION
  + "Crop a fixel index image along with corresponding fixel data images";

  ARGUMENTS
  + Argument ("fixel_in", "the input fixel folder").type_text ()
  + Argument ("fixel_out", "the output fixel folder").type_text ();

  OPTIONS
  + Option ("mask", "the threshold mask").required ()
  + Argument ("data_image").type_image_in ()
  + Option ("data", "specify the list of fixel data images to be cropped relative to the input fixel folder ").allow_multiple ()
  + Argument ("data_images").type_image_in ();
}



void run ()
{
  const auto fixel_folder = argument[0];
  FixelFormat::check_fixel_folder (fixel_folder);

  const auto out_fixel_folder = argument[1];
  auto index_image = FixelFormat::find_index_header (fixel_folder).get_image <uint32_t> ();

  auto opt = get_options ("mask");
  auto mask_image = Image<bool>::open (std::string (opt[0][0]));

  FixelFormat::check_fixel_size (index_image, mask_image);
  FixelFormat::check_fixel_folder (out_fixel_folder, true);

  Header out_header = Header (index_image);
  size_t total_nfixels = std::stoul (out_header.keyval ()[FixelFormat::n_fixels_key]);

  // We need to do a first pass fo the mask image to determine the cropped images' properties
  for (auto l = Loop (0) (mask_image); l; ++l) {
    if (!mask_image.value ())
      total_nfixels --;
  }

  out_header.keyval ()[FixelFormat::n_fixels_key] = str (total_nfixels);
  auto index_image_basename = Path::basename (index_image.name ());
  auto out_index_image = Image<uint32_t>::create (Path::join (out_fixel_folder, index_image_basename), out_header);

  // Crop index images
  mask_image.index (1) = 0;
  for (auto l = Loop ("cropping index image: " + index_image_basename, 0, 3) (index_image, out_index_image); l; ++l) {
    index_image.index (3) = 0;
    size_t nfixels = index_image.value ();

    index_image.index (3) = 1;
    size_t offset = index_image.value ();

    for (size_t i = 0, N = nfixels; i < N; ++i) {
      mask_image.index (0) = offset + i;
      if (!mask_image.value ())
        nfixels--;
    }

    out_index_image.index (3) = 0;
    out_index_image.value () = nfixels;

    out_index_image.index (3) = 1;
    out_index_image.value () = offset;
  }

  // Crop data images
  opt = get_options ("data");

  for (size_t n = 0, N = opt.size (); n < N; n++) {
    const auto data_file = opt[n][0];
    const auto data_file_path = Path::join (fixel_folder, data_file);

    Header header = Header::open (data_file_path);
    auto in_data = header.get_image <float> ();

    check_dimensions (in_data, mask_image, {0, 2});

    header.size (0) = total_nfixels;
    auto out_data = Image<float>::create (Path::join (out_fixel_folder, data_file), header);

    size_t offset(0);

    for (auto l = Loop ("cropping data: " + data_file, 0) (mask_image, in_data); l; ++l) {
      if (mask_image.value ()) {
        out_data.index (1) = offset;
        out_data.row (1) = in_data.row (1);
        offset++;
      }
    }

  }

}

