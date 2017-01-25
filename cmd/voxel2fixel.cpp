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

#include "fixel/helpers.h"
#include "fixel/keys.h"
#include "fixel/loop.h"

using namespace MR;
using namespace App;


void usage ()
{
  AUTHOR = "David Raffelt (david.raffelt@florey.edu.au)";

  DESCRIPTION
  + "map the scalar value in each voxel to all fixels within that voxel. "
    "This is designed to enable CFE-based statistical analysis to be performed on voxel-wise measures";

  ARGUMENTS
  + Argument ("image_in",  "the input image.").type_image_in()
  + Argument ("fixel_directory_in",  "the input fixel directory. Used to define the fixels and their directions").type_text()
  + Argument ("fixel_directory_out", "the output fixel directory. This can be the same as the input directory if desired").type_text()
  + Argument ("fixel_data_out", "the name of the fixel data image.").type_image_out();
}


void run ()
{
  auto scalar = Image<float>::open (argument[0]);
  std::string input_fixel_directory = argument[1];
  Fixel::check_fixel_directory (input_fixel_directory);
  auto input_fixel_index = Fixel::find_index_header (input_fixel_directory).get_image<uint32_t>();
  check_dimensions (scalar, input_fixel_index, 0, 3);

  std::string output_fixel_directory = argument[2];
  if (input_fixel_directory != output_fixel_directory) {
    ProgressBar progress ("copying fixel index and directions file into output directory");
    progress++;
    Fixel::copy_index_and_directions_file (input_fixel_directory, output_fixel_directory);
    progress++;
  }

  auto output_fixel_data = Image<float>::create (Path::join(output_fixel_directory, argument[3]), Fixel::data_header_from_index (input_fixel_index));

  for (auto v = Loop ("mapping voxel scalar values to fixels", 0, 3)(scalar, input_fixel_index); v; ++v) {
    for (auto f = Fixel::Loop (input_fixel_index) (output_fixel_data); f; ++f)
      output_fixel_data.value() = scalar.value();
  }
}
