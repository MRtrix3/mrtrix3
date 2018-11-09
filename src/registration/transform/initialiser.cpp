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


#include "registration/transform/initialiser.h"
#include "registration/transform/initialiser_helpers.h"

namespace MR
{
  namespace Registration
  {
    namespace Transform
    {
      namespace Init
      {
        void set_centre_via_mass (
          Image<default_type>& im1,
          Image<default_type>& im2,
          Image<default_type>& mask1,
          Image<default_type>& mask2,
          Registration::Transform::Base& transform,
          Registration::Transform::Init::LinearInitialisationParams& init) {

          CONSOLE ("initialising centre of rotation using centre of mass");
          Eigen::Vector3 im1_centre_mass, im2_centre_mass;
          Eigen::Vector3 im1_centre_mass_transformed, im2_centre_mass_transformed;

          Image<default_type> bogus_mask;

          // TODO: add option to use mask instead of image intensities

          get_centre_of_mass (im1, init.init_translation.unmasked1 ? bogus_mask : mask1, im1_centre_mass);
          get_centre_of_mass (im2, init.init_translation.unmasked2 ? bogus_mask : mask2, im2_centre_mass);

          transform.transform_half_inverse (im1_centre_mass_transformed, im1_centre_mass);
          transform.transform_half (im2_centre_mass_transformed, im2_centre_mass);

          Eigen::Vector3 centre = (im1_centre_mass + im2_centre_mass) * 0.5;
          DEBUG("centre: " + str(centre.transpose()));
          transform.set_centre_without_transform_update (centre);
        }

        void set_centre_via_image_centres (
          const Image<default_type>& im1,
          const Image<default_type>& im2,
          const Image<default_type>& mask1,
          const Image<default_type>& mask2,
          Registration::Transform::Base& transform,
          Registration::Transform::Init::LinearInitialisationParams& init) {

          CONSOLE ("initialising centre of rotation using geometric centre");
          Eigen::Vector3 im1_centre_scanner;
          get_geometric_centre (im1, im1_centre_scanner);

          Eigen::Vector3 im2_centre_scanner;
          get_geometric_centre (im2, im2_centre_scanner);

          Eigen::Vector3 centre = (im1_centre_scanner + im2_centre_scanner) / 2.0;
          DEBUG("centre: " + str(centre.transpose()));
          transform.set_centre_without_transform_update (centre);
        }


        void initialise_using_image_centres (
          const Image<default_type>& im1,
          const Image<default_type>& im2,
          const Image<default_type>& mask1,
          const Image<default_type>& mask2,
          Registration::Transform::Base& transform,
          Registration::Transform::Init::LinearInitialisationParams& init) {

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

        void initialise_using_image_moments (
          Image<default_type>& im1,
          Image<default_type>& im2,
          Image<default_type>& mask1,
          Image<default_type>& mask2,
          Registration::Transform::Base& transform,
          Registration::Transform::Init::LinearInitialisationParams& init) {

          Image<default_type> bogus_mask;
          bool use_mask_values_instead = false; // TODO add to options
          if (use_mask_values_instead) {
            if (!(mask1.valid() or mask2.valid()))
              throw Exception ("cannot run image moments initialisation using mask values without a valid mask");
            CONSOLE ("initialising using image moments using mask values instead of image values");
          }
          else
            CONSOLE ("initialising using image moments");

          auto moments_init = Transform::Init::MomentsInitialiser (
            im1,
            im2,
            init.init_rotation.unmasked1 ? bogus_mask : mask1,
            init.init_rotation.unmasked2 ? bogus_mask : mask2,
            transform,
            use_mask_values_instead);
          moments_init.run();
        }


        void initialise_using_FOD (
          Image<default_type>& im1,
          Image<default_type>& im2,
          Image<default_type>& mask1,
          Image<default_type>& mask2,
          Registration::Transform::Base& transform,
          Registration::Transform::Init::LinearInitialisationParams& init) {

          Image<default_type> bogus_mask;
          ssize_t lmax = -1; // TODO add to options
          CONSOLE ("initialising using masked images interpreted as FOD");
          WARN ("Not implemented yet. Setting only centre of mass.");
          auto fod_init = Transform::Init::FODInitialiser (
            im1,
            im2,
            init.init_rotation.unmasked1 ? bogus_mask : mask1,
            init.init_rotation.unmasked2 ? bogus_mask : mask2,
            transform,
            lmax);
          fod_init.run();
        }

        void initialise_using_rotation_search (
          Image<default_type>& im1,
          Image<default_type>& im2,
          Image<default_type>& mask1,
          Image<default_type>& mask2,
          Registration::Transform::Base& transform,
          Registration::Transform::Init::LinearInitialisationParams& init);

        void initialise_using_image_mass (
          Image<default_type>& im1,
          Image<default_type>& im2,
          Image<default_type>& mask1,
          Image<default_type>& mask2,
          Registration::Transform::Base& transform,
          Registration::Transform::Init::LinearInitialisationParams& init);
      }
    }
  }
}

