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
        enum InitType {mass, geometric, moments, mass_unmasked, moments_unmasked, none};


        template <class Im1ImageType,
                  class Im2ImageType,
                  class TransformType>
          void initialise_using_image_centres (const Im1ImageType& im1,
                                               const Im2ImageType& im2,
                                               TransformType& transform) {
            CONSOLE ("initialising centre of rotation and translation using geometric centre");
            Eigen::Vector3 im1_centre_scanner;
            get_geometric_centre (im1, im1_centre_scanner);

            Eigen::Vector3 im2_centre_scanner;
            get_geometric_centre (im2, im2_centre_scanner);

            Eigen::Vector3 translation = im1_centre_scanner - im2_centre_scanner;
            Eigen::Vector3 centre = (im1_centre_scanner + im2_centre_scanner) / 2.0;
            transform.set_centre (centre);
            transform.set_translation (translation);
          }

        void initialise_using_image_moments (Image<default_type>& im1,
                                             Image<default_type>& im2,
                                             Image<default_type>& mask1,
                                             Image<default_type>& mask2,
                                             Registration::Transform::Base& transform) {
          CONSOLE ("initialising using image moments");
          auto init = Transform::Init::MomentsInitialiser (im1, im2, mask1, mask2, transform);
          init.run();
        }

        void initialise_using_image_moments (Image<default_type>& im1,
                                             Image<default_type>& im2,
                                             Registration::Transform::Base& transform) {
          CONSOLE ("initialising using image moments with unmasked images");
          Image<default_type> mask1;
          Image<default_type> mask2;
          auto init = Transform::Init::MomentsInitialiser (im1, im2, mask1, mask2, transform);
          init.run();
        }

        void initialise_using_image_mass (Image<default_type>& im1,
                                          Image<default_type>& im2,
                                          Image<default_type>& mask1,
                                          Image<default_type>& mask2,
                                          Registration::Transform::Base& transform);

        void initialise_using_image_mass (Image<default_type>& im1,
                                          Image<default_type>& im2,
                                          Registration::Transform::Base& transform) {
          Image<default_type> bogus_mask;
          initialise_using_image_mass (im1, im2, bogus_mask, bogus_mask, transform);
        }
      }
    }
  }
}

#endif
