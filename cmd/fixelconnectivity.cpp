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
#include "fixel/helpers.h"
#include "fixel/index_remapper.h"
#include "fixel/matrix.h"


#define DEFAULT_ANGLE_THRESHOLD 45.0
#define DEFAULT_CONNECTIVITY_THRESHOLD 0.01
#define DEFAULT_SMOOTHING_FWHM 10.0


using namespace MR;
using namespace App;

void usage ()
{
  AUTHOR = "Robert E. Smith (robert.smith@florey.edu.au)";

  SYNOPSIS = "Generate one or more fixel-fixel connectivity matrices";

  DESCRIPTION
  + "If this command is used within a typical Fixel-Based Analysis (FBA) pipeline, one would be expected to "
    "utilise the -smoothing command-line option, as this will provide the matrix by which fixel data should be "
    "smoothed in template space, as opposed to statistical testing which would typically use the full output "
    "connectivity matrix";

  ARGUMENTS
  + Argument ("fixel_directory", "the directory containing the fixels between which connectivity will be quantified").type_directory_in()

  + Argument ("tracks", "the tracks used to determine fixel-fixel connectivity").type_tracks_in()

  + Argument ("matrix", "the output fixel-fixel connectivity matrix").type_file_out();


  OPTIONS

  + OptionGroup ("Options for generating an additional connectivity matrix")

  + Option ("smoothing", "generate a second fixel-fixel connectivity matrix that incorporates both streamlines-based connectivity and spatial distance, "
                         "which is typically used for smoothing fixel-based data")
    + Argument ("path").type_file_out()

  + Option ("fwhm", "manually specify the full-width half-maximum of the fixel data smoothing matrix (default: " + str(DEFAULT_SMOOTHING_FWHM, 2) + "mm)")
    + Argument ("value").type_float (0.0, 200.0)

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
  const bool do_smoothing = get_options ("smoothing").size();
  value_type smoothing_fwhm = value_type (0);
  if (do_smoothing) {
    smoothing_fwhm = get_option_value ("fwhm", DEFAULT_SMOOTHING_FWHM) / 2.3548;
  } else if (get_options ("fwhm").size()) {
    WARN ("Usage of -fwhm ignored, since no output smoothing matrix path has been specified");
  }
  const value_type connectivity_threshold = get_option_value ("connectivity", DEFAULT_CONNECTIVITY_THRESHOLD);
  const value_type angular_threshold = get_option_value ("angle", DEFAULT_ANGLE_THRESHOLD);

  const std::string input_fixel_directory = argument[0];
  Header index_header = Fixel::find_index_header (input_fixel_directory);
  auto index_image = index_header.get_image<index_type>();
  const index_type num_fixels = Fixel::get_number_of_fixels (index_image);

  // Regardless of whether or not a mask is provided, we do not want the
  //   normalise_matrix() function to alter the indices of fixels
  const Fixel::IndexRemapper index_remapper (num_fixels);

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

  Fixel::Matrix::norm_matrix_type norm_connectivity_matrix;
  Fixel::Matrix::norm_matrix_type smoothing_weights;
  Fixel::Matrix::normalise (connectivity_matrix,
                            index_image,
                            index_remapper,
                            connectivity_threshold,
                            norm_connectivity_matrix,
                            smoothing_fwhm,
                            smoothing_weights);

  Fixel::Matrix::save (norm_connectivity_matrix, argument[2]);
  if (smoothing_fwhm) {
    opt = get_options ("smoothing");
    assert (!opt.empty());
    Fixel::Matrix::save (smoothing_weights, opt[0][0]);
  }

}

