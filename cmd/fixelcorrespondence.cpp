/*
    Copyright 2014 Brain Research Institute, Melbourne, Australia

    Written by David Raffelt, 2014

    This file is part of MRtrix.

    MRtrix is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    MRtrix is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with MRtrix.  If not, see <http://www.gnu.org/licenses/>.

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

void usage ()
{
  AUTHOR = "David Raffelt (david.raffelt@florey.edu.au)";

  DESCRIPTION
  + "Obtain angular correpondence by mapping subject fixels to a template fixel mask. "
    "The subject fixel image must be already in the same space as the template";

  ARGUMENTS
  + Argument ("subject", "the input subject fixel image.").type_image_in ()
  + Argument ("template", "the input template fixel image.").type_image_in ()
  + Argument ("output", "the input fixel image.").type_image_out ();

  OPTIONS
  + Option ("angle", "the max angle threshold for computing inter-subject fixel correspondence (Default: 30)")
  + Argument ("value").type_float (0.0, 30, 90);
}


void run ()
{

  Sparse::Image<FixelMetric> subject_fixel (argument[0]);

  auto template_header = Header::open (argument[1]);
  Sparse::Image<FixelMetric> template_fixel (argument[1]);

  check_dimensions (subject_fixel, template_fixel);

  Sparse::Image<FixelMetric> output_fixel (argument[2], template_header);

  float angular_threshold = 30.0;
  auto opt = get_options ("angle");
  if (opt.size())
    angular_threshold = opt[0][0];
  const float angular_threshold_dp = cos (angular_threshold * (Math::pi/180.0));

  for (auto i = Loop ("mapping subject fixels to template fixels", subject_fixel) (subject_fixel, template_fixel, output_fixel); i; ++i) {
    output_fixel.value().set_size (template_fixel.value().size());
    for (size_t t = 0; t != template_fixel.value().size(); ++t) {
      output_fixel.value()[t] = template_fixel.value()[t] ;
      float largest_dp = 0.0;
      int index_of_closest_fixel = -1;
      for (size_t s = 0; s != subject_fixel.value().size(); ++s) {
        float dp = std::abs (template_fixel.value()[t].dir.dot (subject_fixel.value()[s].dir));
        if (dp > largest_dp) {
          largest_dp = dp;
          index_of_closest_fixel = s;
        }
      }
      if (largest_dp > angular_threshold_dp) {
        output_fixel.value()[t].value  = subject_fixel.value()[index_of_closest_fixel].value;
      }
    }
  }

}

