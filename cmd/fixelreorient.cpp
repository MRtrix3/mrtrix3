/* Copyright (c) 2008-2017 the MRtrix3 contributors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, you can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * MRtrix is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * For more details, see http://www.mrtrix.org/.
 */


#include "command.h"
#include "progressbar.h"
#include "algo/loop.h"
#include "image.h"
#include "adapter/jacobian.h"
#include "registration/warp/helpers.h"

using namespace MR;
using namespace App;

#include "fixel/helpers.h"
#include "fixel/keys.h"

void usage ()
{
  AUTHOR = "David Raffelt (david.raffelt@florey.edu.au)";

  DESCRIPTION
  + "Reorient fixel directions. Reorientation is performed by transforming the vector representing "
    "the fixel direction with the Jacobian (local affine transform) computed at each voxel in the warp, "
    "then re-normalising the vector.";

  ARGUMENTS
  + Argument ("fixel_in", "the fixel directory").type_text ()
  + Argument ("warp", "a 4D deformation field used to perform reorientation. "
                      "Reorientation is performed by applying the Jacobian affine transform in each voxel in the warp, "
                      "then re-normalising the vector representing the fixel direction").type_image_in ()
  + Argument ("fixel_out", "the output fixel directory. If the the input and output directorys are the same, the existing directions file will "
                           "be replaced (providing the --force option is supplied). If a new directory is supplied then the fixel directions and all "
                           "other fixel data will be copied to the new directory.").type_text ();
}


void run ()
{
  std::string input_fixel_directory = argument[0];
  Fixel::check_fixel_directory (input_fixel_directory);

  auto input_index_image = Fixel::find_index_header (input_fixel_directory).get_image <uint32_t>();

  Header warp_header = Header::open (argument[1]);
  Registration::Warp::check_warp (warp_header);
  check_dimensions (input_index_image, warp_header, 0, 3);
  Adapter::Jacobian<Image<float> > jacobian (warp_header.get_image<float>());

  std::string output_fixel_directory = argument[2];
  Fixel::check_fixel_directory (output_fixel_directory, true);

  // scratch buffer so inplace reorientation can be performed if desired
  Image<float> input_directions_image;
  std::string output_directions_filename;
  {
    auto tmp = Fixel::find_directions_header (input_fixel_directory).get_image<float>();
    input_directions_image = Image<float>::scratch(tmp);
    threaded_copy (tmp, input_directions_image);
    output_directions_filename = Path::basename(tmp.name());
  }

  auto output_directions_image = Image<float>::create (Path::join(output_fixel_directory, output_directions_filename), input_directions_image).with_direct_io();

  for (auto i = Loop ("reorienting fixel directions", input_index_image, 0, 3)(input_index_image, jacobian); i; ++i) {
    input_index_image.index(3) = 0;
    uint32_t num_fixels_in_voxel = input_index_image.value();
    if (num_fixels_in_voxel) {
      input_index_image.index(3) = 1;
      uint32_t index = input_index_image.value();
      Eigen::Matrix<float, 3, 3> transform = jacobian.value().inverse();
      for (size_t f = 0; f < num_fixels_in_voxel; ++f) {
        input_directions_image.index(0) = index + f;
        output_directions_image.index(0) = index + f;
        output_directions_image.row(1) = (transform * Eigen::Vector3f (input_directions_image.row(1))).normalized();
      }
    }
  }

  if (output_fixel_directory != input_fixel_directory) {
    Fixel::copy_index_file (input_fixel_directory, output_fixel_directory);
    Fixel::copy_all_data_files (input_fixel_directory, output_fixel_directory);
  }
}

