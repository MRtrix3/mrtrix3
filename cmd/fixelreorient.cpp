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
#include "adapter/jacobian.h"

using namespace MR;
using namespace App;

using Sparse::FixelMetric;

void usage ()
{
  AUTHOR = "David Raffelt (david.raffelt@florey.edu.au)";

  DESCRIPTION
  + "Reorient fixel directions using the local affine transformation (Jacobian matrix) of an input warp.";

  ARGUMENTS
  + Argument ("input", "the input fixel image.").type_image_in ()
  + Argument ("warp", "a 4D deformation field used to perform reorientation. "
                         "Reorientation is performed by applying the Jacobian affine transform in each voxel in the warp, "
                         "then re-normalising the vector representing the fixel direction").type_image_in ()
  + Argument ("output", "the output fixel image.").type_image_out ();
}


void run ()
{
  auto input_header = Header::open (argument[0]);
  Sparse::Image<FixelMetric> input_fixel (argument[0]);

  Header warp_header = Header::open(argument[1]);

  check_dimensions (input_fixel, warp_header, 0, 3);

  if (warp_header.ndim() != 4)
    throw Exception ("The input deformation field image must be a 4D file.");
  if (warp_header.size(3) != 3)
  check_dimensions (warp_header, input_fixel, 0, 3);
  Adapter::Jacobian<Image<float> > jacobian (warp_header.get_image<float>());
  Sparse::Image<FixelMetric> output_fixel (argument[2], input_header);

  for (auto i = Loop ("reorienting fixel directions", input_fixel) (input_fixel, jacobian, output_fixel); i; ++i) {
    output_fixel.value().set_size (input_fixel.value().size());
    for (size_t f = 0; f != input_fixel.value().size(); ++f) {
      output_fixel.value()[f] = input_fixel.value()[f];
      Eigen::Vector3f subject_fixel_direction =  jacobian.value().inverse() * input_fixel.value()[f].dir;
      subject_fixel_direction.normalize();
      output_fixel.value()[f].dir = subject_fixel_direction;
    }
  }
}

