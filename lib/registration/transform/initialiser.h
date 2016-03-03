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

#ifndef __registration_transform_initialiser_h__
#define __registration_transform_initialiser_h__

#include "image.h"
#include "transform.h"
#include "registration/transform/base.h"
#include "registration/transform/initialiser_helpers.h"

namespace MR
{
  namespace Registration
  {
    namespace Transform
    {
      namespace Init
      {
        enum InitType {mass, geometric, moments, mass_unmasked, moments_use_mask_intensity,
          moments_unmasked, fod, set_centre_mass, rot_search, none};

          extern void set_centre_using_image_mass (Image<default_type>& im1,
                                            Image<default_type>& im2,
                                            Image<default_type>& mask1,
                                            Image<default_type>& mask2,
                                            Registration::Transform::Base& transform);

        extern void initialise_using_image_centres (const Image<default_type>& im1,
                                               const Image<default_type>& im2,
                                               Registration::Transform::Base& transform);

        extern void initialise_using_image_moments (Image<default_type>& im1,
                                             Image<default_type>& im2,
                                             Image<default_type>& mask1,
                                             Image<default_type>& mask2,
                                             Registration::Transform::Base& transform,
                                             bool use_mask_values = false);

        extern void initialise_using_image_moments (Image<default_type>& im1,
                                             Image<default_type>& im2,
                                             Registration::Transform::Base& transform);

        extern void initialise_using_FOD (Image<default_type>& im1,
                                   Image<default_type>& im2,
                                   Image<default_type>& mask1,
                                   Image<default_type>& mask2,
                                   Registration::Transform::Base& transform,
                                   ssize_t lmax = -1);


        extern void initialise_using_FOD (Image<default_type>& im1,
                                   Image<default_type>& im2,
                                   Registration::Transform::Base& transform,
                                   ssize_t lmax = -1);

        extern void initialise_using_rotation_search_around_image_mass (
                                          Image<default_type>& im1,
                                          Image<default_type>& im2,
                                          Image<default_type>& mask1,
                                          Image<default_type>& mask2,
                                          Registration::Transform::Base& transform,
                                          default_type image_scale = 0.1,
                                          bool global_search = false,
                                          bool debug = false);

        extern void initialise_using_image_mass (Image<default_type>& im1,
                                          Image<default_type>& im2,
                                          Image<default_type>& mask1,
                                          Image<default_type>& mask2,
                                          Registration::Transform::Base& transform);

        extern void initialise_using_image_mass (Image<default_type>& im1,
                                          Image<default_type>& im2,
                                          Registration::Transform::Base& transform);
      }
    }
  }
}

#endif
