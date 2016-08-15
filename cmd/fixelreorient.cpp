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

#include "fixel_format/helpers.h"
#include "fixel_format/keys.h"

void usage ()
{
  AUTHOR = "David Raffelt (david.raffelt@florey.edu.au)";

  DESCRIPTION
  + "Inplace reorientation of fixel directions using the local affine transformation (Jacobian matrix) at each voxel within an input warp.";

  ARGUMENTS
  + Argument ("fixel_in", "the fixel folder").type_text ()
  + Argument ("warp", "a 4D deformation field used to perform reorientation. "
                      "Reorientation is performed by applying the Jacobian affine transform in each voxel in the warp, "
                      "then re-normalising the vector representing the fixel direction").type_image_in ()
  + Argument ("dir_out", "the output fixel folder. If the the input and output folders are the same the directions file will "
                         "replaced. If a new folder is supplied then all fixel data will be copied to the new folder.").type_text ();
}


void run ()
{
  const auto fixel_folder = argument[0];
  FixelFormat::check_fixel_folder (fixel_folder);

  Header warp_header = Header::open(argument[1]);

  const auto out_fixel_folder = argument[2];
  auto index_image = FixelFormat::find_index_header (fixel_folder).get_image <uint32_t> ();

  check_dimensions (index_image, warp_header, 0, 3);

  if (warp_header.ndim() != 4)
    throw Exception ("The input deformation field image must be a 4D file.");
  if (warp_header.size(3) != 3)
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

