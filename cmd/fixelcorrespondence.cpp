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

using namespace MR;
using namespace App;

#define DEFAULT_ANGLE_THRESHOLD 45.0

void usage ()
{
  AUTHOR = "David Raffelt (david.raffelt@florey.edu.au)";

  SYNOPSIS = "Obtain fixel-fixel correpondence between a subject fixel image and a template fixel mask";

  DESCRIPTION
  + "It is assumed that the subject image has already been spatially normalised and is aligned with the template. "
    "The output fixel image will have the same fixels (and directions) of the template.";

  ARGUMENTS
  + Argument ("subject_data", "the input subject fixel data file. This should be a file inside the fixel directory").type_image_in ()
  + Argument ("template_directory", "the input template fixel directory.").type_image_in ()
  + Argument ("output_directory", "the output fixel directory.").type_text()
  + Argument ("output_data", "the name of the output fixel data file. This will be placed in the output fixel directory").type_image_out ();

  OPTIONS
  + Option ("angle", "the max angle threshold for computing inter-subject fixel correspondence (Default: " + str(DEFAULT_ANGLE_THRESHOLD, 2) + " degrees)")
  + Argument ("value").type_float (0.0, 90.0);
}


void run ()
{
  const float angular_threshold = get_option_value ("angle", DEFAULT_ANGLE_THRESHOLD);
  const float angular_threshold_dp = cos (angular_threshold * (Math::pi/180.0));

  const std::string input_file (argument[0]);
  if (Path::is_dir (input_file))
    throw Exception ("please input the specific fixel data file to be converted (not the fixel directory)");

  auto subject_index = Fixel::find_index_header (Fixel::get_fixel_directory (input_file)).get_image<uint32_t>();
  auto subject_directions = Fixel::find_directions_header (Fixel::get_fixel_directory (input_file)).get_image<float>().with_direct_io();

  if (input_file == subject_directions.name())
    throw Exception ("input fixel data file cannot be the directions file");

  auto subject_data = Image<float>::open (input_file);
  Fixel::check_fixel_size (subject_index, subject_data);

  auto template_index = Fixel::find_index_header (argument[1]).get_image<uint32_t>();
  auto template_directions = Fixel::find_directions_header (argument[1]).get_image<float>().with_direct_io();

  check_dimensions (subject_index, template_index);
  std::string output_fixel_directory = argument[2];
  Fixel::copy_index_and_directions_file (argument[1], output_fixel_directory);

  Header output_data_header (template_directions);
  output_data_header.size(1) = 1;
  auto output_data = Image<float>::create (Path::join (output_fixel_directory, argument[3]), output_data_header);

  for (auto i = Loop ("mapping subject fixels data to template fixels", template_index, 0, 3)(template_index, subject_index); i; ++i) {
    template_index.index(3) = 0;
    uint32_t nfixels_template = template_index.value();
    template_index.index(3) = 1;
    uint32_t template_offset = template_index.value();

    for (size_t t = 0; t < nfixels_template; ++t) {

      float largest_dp = 0.0;
      int index_of_closest_fixel = -1;

      subject_index.index(3) = 0;
      uint32_t nfixels_subject = subject_index.value();
      subject_index.index(3) = 1;
      uint32_t subject_offset = subject_index.value();

      template_directions.index(0) = template_offset + t;
      for (size_t s = 0; s < nfixels_subject; ++s) {
        subject_directions.index(0) = subject_offset + s;

        Eigen::Vector3f templatedir = template_directions.row(1);
        templatedir.normalize();
        Eigen::Vector3f subjectdir = subject_directions.row(1);
        subjectdir.normalize();
        float dp = std::abs (templatedir.dot (subjectdir));
        if (dp > largest_dp) {
          largest_dp = dp;
          index_of_closest_fixel = s;
        }
      }
      if (largest_dp > angular_threshold_dp) {
        output_data.index(0) = template_offset + t;
        subject_data.index(0) = subject_offset + index_of_closest_fixel;
        output_data.value() = subject_data.value();
      }
    }
  }
}

