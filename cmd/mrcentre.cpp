/* Copyright (c) 2008-2018 the MRtrix3 contributors.
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
#include "image.h"
#include "image_helpers.h"
#include "transform.h"
#include "types.h"

using namespace MR;
using namespace App;


void usage ()
{
  AUTHOR = "Robert E. Smith (robert.smith@florey.edu.au)";

  SYNOPSIS = "Determine the centre of mass / centre of gravity of an image";

  ARGUMENTS
  + Argument ("input", "the input image").type_image_in();

  OPTIONS
  + Option ("mask", "only include voxels within a mask in the calculation")
    + Argument ("image").type_image_in()

  + Option ("voxelspace", "report image centre of mass in voxel space rather than scanner space");

}


typedef float value_type;


void run ()
{
  Image<value_type> image = Image<value_type>::open (argument[0]);
  if (image.ndim() > 3)
    throw Exception ("Command does not accept images with more than 3 dimensions");

  Image<bool> mask;
  auto opt = get_options ("mask");
  if (opt.size()) {
    mask = Image<bool>::open (opt[0][0]);
    check_dimensions (image, mask);
  }

  Eigen::Vector3 com (0.0, 0.0, 0.0);
  default_type mass = 0.0;
  if (mask.valid()) {
    for (auto l = Loop(image) (image, mask); l; ++l) {
      if (mask.value()) {
        com += Eigen::Vector3 (image.index(0), image.index(1), image.index(2)) * image.value();
        mass += image.value();
      }
    }
  } else {
    for (auto l = Loop(image) (image); l; ++l) {
      com += Eigen::Vector3 (image.index(0), image.index(1), image.index(2)) * image.value();
      mass += image.value();
    }
  }

  com /= mass;
  if (!get_options ("voxelspace").size())
    com = image.transform() * com;

  std::cout << com.transpose();
}
