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
#include "fixel/helpers.h"
#include "fixel/index_remapper.h"
#include "fixel/matrix.h"


#define DEFAULT_ANGLE_THRESHOLD 45.0
#define DEFAULT_CONNECTIVITY_THRESHOLD 0.01


using namespace MR;
using namespace App;

void usage ()
{
  AUTHOR = "Robert E. Smith (robert.smith@florey.edu.au)";

  SYNOPSIS = "Generate a fixel-fixel connectivity matrix";

  DESCRIPTION
  + "This command will generate a directory containing three images, which encodes the "
    "fixel-fixel connectivity matrix. Documentation regarding this format and how to "
    "use it will come in the future.";

  ARGUMENTS
  + Argument ("fixel_directory", "the directory containing the fixels between which connectivity will be quantified").type_directory_in()

  + Argument ("tracks", "the tracks used to determine fixel-fixel connectivity").type_tracks_in()

  + Argument ("matrix", "the output fixel-fixel connectivity matrix directory path").type_directory_out();


  OPTIONS
  + OptionGroup ("Options that influence generation of the connectivity matrix / matrices")

  + Option ("threshold", "a threshold to define the required fraction of shared connections to be included in the neighbourhood (default: " + str(DEFAULT_CONNECTIVITY_THRESHOLD, 2) + ")")
    + Argument ("value").type_float (0.0, 1.0)

  + Option ("angle", "the max angle threshold for assigning streamline tangents to fixels (Default: " + str(DEFAULT_ANGLE_THRESHOLD, 2) + " degrees)")
    + Argument ("value").type_float (0.0, 90.0)

  + Option ("mask", "provide a fixel data file containing a mask of those fixels to be computed; fixels outside the mask will be empty in the output matrix")
    + Argument ("file").type_image_in();

}


using value_type = float;
using Fixel::index_type;


void run()
{
  const value_type connectivity_threshold = get_option_value ("connectivity", value_type(DEFAULT_CONNECTIVITY_THRESHOLD));
  const value_type angular_threshold = get_option_value ("angle", value_type(DEFAULT_ANGLE_THRESHOLD));

  const std::string input_fixel_directory = argument[0];
  Header index_header = Fixel::find_index_header (input_fixel_directory);
  auto index_image = index_header.get_image<index_type>();
  const index_type num_fixels = Fixel::get_number_of_fixels (index_image);

  // When provided with a mask, this only influences which fixels get their connectivity quantified;
  //   these will appear empty in the output matrix
  auto opt = get_options ("mask");
  Image<bool> fixel_mask;
  if (opt.size()) {
    fixel_mask = Image<bool>::open (opt[0][0]);
    Fixel::check_data_file (fixel_mask);
    if (!Fixel::fixels_match (index_header, fixel_mask))
      throw Exception ("Mask image provided using -mask option does not match input fixel directory");
  } else {
    Header mask_header = Fixel::data_header_from_index (index_header);
    mask_header.datatype() = DataType::Bit;
    fixel_mask = Image<bool>::scratch (mask_header, "true-filled scratch fixel mask");
    for (fixel_mask.index(0) = 0; fixel_mask.index(0) != num_fixels; ++fixel_mask.index(0))
      fixel_mask.value() = true;
  }

  auto connectivity_matrix = Fixel::Matrix::generate (argument[1],
                                                      index_image,
                                                      fixel_mask,
                                                      angular_threshold);

  Fixel::Matrix::normalise_and_write (connectivity_matrix,
                                      connectivity_threshold,
                                      argument[2]);

}

