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
#include "image.h"
#include "transform.h"
// #include "registration/metric/mean_squared.h"
#include "registration/multi_contrast.h"
#include "registration/transform/affine.h"
// #include "registration/metric/difference_robust.h"
// #include "registration/metric/normalised_cross_correlation.h"
#include "math/average_space.h"
#include "transform.h"

#include "registration/transform/initialiser.h"
#include "registration/transform/search.h"


using namespace MR;
using namespace App;

const OptionGroup rot_options =
      OptionGroup ("rotation search options")

      + Option ("init_rotation.search.angles", "rotation angles for the local search in degrees between 0 and 180. "
                                  "(Default: 2,5,10,15,20)")
        + Argument ("angles").type_sequence_float ()
      + Option ("init_rotation.search.scale", "relative size of the images used for the rotation search. (Default: 0.15)")
        + Argument ("scale").type_float (0.0001, 1.0)
      + Option ("init_rotation.search.directions", "number of rotation axis for local search. (Default: 250)")
        + Argument ("num").type_integer (1, 10000)
      + Option ("init_rotation.search.run_global", "perform a global search. (Default: local)")
      + Option ("init_rotation.search.global.iterations", "number of rotations to investigate (Default: 10000)")
        + Argument ("num").type_integer (1, 1e10);

void usage ()
{
  AUTHOR = "Max Pietsch (maximilian.pietsch@kcl.ac.uk)";

  SYNOPSIS = "Calculate the transformation required to align two images";

  DESCRIPTION
      + "TODO";


  ARGUMENTS
    + Argument ("image1").type_image_in()
    + Argument ("image2").type_image_in()
    + Argument ("transformation").type_file_out();

  OPTIONS
  + Option ("mask1", "a mask to define the region of image1")
    + Argument ("filename").type_image_in ()

  + Option ("mask2", "a mask to define the region of image2")
    + Argument ("filename").type_image_in ()

  + Option ("moments", " ")
  + Option ("rotation", " ")

  + rot_options;

}

using value_type = double;

void run () {

  auto im1_image = Image<value_type>::open(argument[0]);
  auto im2_image = Image<value_type>::open(argument[1]);


  auto opt = get_options ("mask1");
  Image<value_type> im1_mask;
  if (opt.size ()) {
    im1_mask = Image<value_type>::open(opt[0][0]);
    check_dimensions (im1_image, im1_mask, 0, 3);
  }

  opt = get_options ("mask2");
  Image<value_type> im2_mask;
  if (opt.size ()) {
    im2_mask = Image<value_type>::open(opt[0][0]);
    check_dimensions (im2_image, im2_mask, 0, 3);
  }

  Registration::Transform::Init::LinearInitialisationParams init;
  Registration::Transform::Affine transform;

  vector<Registration::MultiContrastSetting> contrasts;

  // if (init_translation_type == Transform::Init::mass)
  if (get_options ("moments").size()) {
    Registration::Transform::Init::initialise_using_image_moments (im1_image, im2_image, im1_mask, im2_mask, transform, init, contrasts);
  }
  else if (get_options ("rotation").size()) {
    Registration::Transform::Init::initialise_using_image_mass (im1_image, im2_image, im1_mask, im2_mask, transform, init, contrasts);
    Registration::Transform::Init::initialise_using_rotation_search (im1_image, im2_image, im1_mask, im2_mask, transform, init, contrasts);
  }
  else {
    Registration::Transform::Init::initialise_using_image_mass (im1_image, im2_image, im1_mask, im2_mask, transform, init, contrasts);
  }
  INFO(transform.info());
  // Registration::Transform::Init::initialise_using_image_centres (im1_image, im2_image, im1_mask, im2_mask, transform, init);
  // Registration::Transform::Init::set_centre_via_mass (im1_image, im2_image, im1_mask, im2_mask, transform, init, contrasts);
  // Registration::Transform::Init::set_centre_via_image_centres (im1_image, im2_image, im1_mask, im2_mask, transform, init);
  save_transform (transform.get_transform(), argument[2]);
  std::cout << transform.get_centre()(0) << ',' << transform.get_centre()(1) << ',' << transform.get_centre()(2) << std::endl;
}
