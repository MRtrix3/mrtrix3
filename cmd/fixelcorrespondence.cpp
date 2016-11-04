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
#include "sparse/fixel_metric.h"
#include "sparse/image.h"

using namespace MR;
using namespace App;

using Sparse::FixelMetric;

#define DEFAULT_ANGLE_THRESHOLD 45.0

void usage ()
{
  AUTHOR = "David Raffelt (david.raffelt@florey.edu.au)";

  DESCRIPTION
  + "Obtain angular correpondence by mapping subject fixels to a template fixel mask. "
    "It is assumed that the subject image has already been spatially normalised and is aligned with the template.";

  ARGUMENTS
  + Argument ("subject", "the input subject fixel image.").type_image_in ()
  + Argument ("template", "the input template fixel image.").type_image_in ()
  + Argument ("output", "the output fixel image.").type_image_out ();

  OPTIONS
  + Option ("angle", "the max angle threshold for computing inter-subject fixel correspondence (Default: " + str(DEFAULT_ANGLE_THRESHOLD, 2) + " degrees)")
  + Argument ("value").type_float (0.0, 90.0);
}


void run ()
{

  Sparse::Image<FixelMetric> subject_fixel (argument[0]);

  auto template_header = Header::open (argument[1]);
  Sparse::Image<FixelMetric> template_fixel (argument[1]);

  check_dimensions (subject_fixel, template_fixel);

  Sparse::Image<FixelMetric> output_fixel (argument[2], template_header);

  const float angular_threshold = get_option_value ("angle", DEFAULT_ANGLE_THRESHOLD);
  const float angular_threshold_dp = cos (angular_threshold * (Math::pi/180.0));

  for (auto i = Loop ("mapping subject fixels to template fixels", subject_fixel) (subject_fixel, template_fixel, output_fixel); i; ++i) {
    output_fixel.value().set_size (template_fixel.value().size());
    for (size_t t = 0; t != template_fixel.value().size(); ++t) {
      output_fixel.value()[t] = template_fixel.value()[t] ;
      float largest_dp = 0.0;
      int index_of_closest_fixel = -1;

      for (size_t s = 0; s != subject_fixel.value().size(); ++s) {
        Eigen::Vector3f template_dir = template_fixel.value()[t].dir;
        template_dir.normalize();
        Eigen::Vector3f subject_dir = subject_fixel.value()[s].dir;
        subject_dir.normalize();

        float dp = std::abs (template_dir.dot (subject_dir));

        if (dp > largest_dp) {
          largest_dp = dp;
          index_of_closest_fixel = s;
        }
      }
      if (largest_dp > angular_threshold_dp) {
        output_fixel.value()[t].value  = subject_fixel.value()[index_of_closest_fixel].value;
      } else {
        output_fixel.value()[t].value = 0.0;
      }
    }
  }

}

