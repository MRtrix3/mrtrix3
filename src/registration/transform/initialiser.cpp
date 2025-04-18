/* Copyright (c) 2008-2025 the MRtrix3 contributors.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Covered Software is provided under this License on an "as is"
 * basis, without warranty of any kind, either expressed, implied, or
 * statutory, including, without limitation, warranties that the
 * Covered Software is free of defects, merchantable, fit for a
 * particular purpose or non-infringing.
 * See the Mozilla Public License v. 2.0 for more details.
 *
 * For more details, see http://www.mrtrix.org/.
 */

#include "registration/transform/initialiser.h"
#include "registration/transform/initialiser_helpers.h"
#include "registration/multi_contrast.h"

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
          Registration::Transform::Init::LinearInitialisationParams& init,
          const vector<MultiContrastSetting>& contrast_settings) {

          CONSOLE ("initialising centre of rotation using centre of mass");
          Eigen::Vector3d im1_centre_mass, im2_centre_mass;
          Eigen::Vector3d im1_centre_mass_transformed, im2_centre_mass_transformed;

          Image<default_type> bogus_mask;

          get_centre_of_mass (im1, init.init_translation.unmasked1 ? bogus_mask : mask1, im1_centre_mass, contrast_settings);
          get_centre_of_mass (im2, init.init_translation.unmasked2 ? bogus_mask : mask2, im2_centre_mass, contrast_settings);

          transform.transform_half_inverse (im1_centre_mass_transformed, im1_centre_mass);
          transform.transform_half (im2_centre_mass_transformed, im2_centre_mass);

          Eigen::Vector3d centre = (im1_centre_mass + im2_centre_mass) * 0.5;
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
          Eigen::Vector3d im1_centre_scanner;
          get_geometric_centre (im1, im1_centre_scanner);

          Eigen::Vector3d im2_centre_scanner;
          get_geometric_centre (im2, im2_centre_scanner);

          Eigen::Vector3d centre = (im1_centre_scanner + im2_centre_scanner) / 2.0;
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
          Eigen::Vector3d im1_centre_scanner;
          get_geometric_centre (im1, im1_centre_scanner);

          Eigen::Vector3d im2_centre_scanner;
          get_geometric_centre (im2, im2_centre_scanner);

          Eigen::Vector3d translation = im1_centre_scanner - im2_centre_scanner;
          Eigen::Vector3d centre = (im1_centre_scanner + im2_centre_scanner) / 2.0;
          transform.set_centre (centre);
          transform.set_translation (translation);
        }

        void initialise_using_image_moments (
          Image<default_type>& im1,
          Image<default_type>& im2,
          Image<default_type>& mask1,
          Image<default_type>& mask2,
          Registration::Transform::Base& transform,
          Registration::Transform::Init::LinearInitialisationParams& init,
          const vector<MultiContrastSetting>& contrast_settings) {

          Image<default_type> bogus_mask;
          CONSOLE ("initialising using image moments");

          auto moments_init = Transform::Init::MomentsInitialiser (
            im1,
            im2,
            init.init_rotation.unmasked1 ? bogus_mask : mask1,
            init.init_rotation.unmasked2 ? bogus_mask : mask2,
            transform,
            contrast_settings);
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
          Registration::Transform::Init::LinearInitialisationParams& init,
          const vector<MultiContrastSetting>& contrast_settings);

        void initialise_using_image_mass (
          Image<default_type>& im1,
          Image<default_type>& im2,
          Image<default_type>& mask1,
          Image<default_type>& mask2,
          Registration::Transform::Base& transform,
          Registration::Transform::Init::LinearInitialisationParams& init,
          const vector<MultiContrastSetting>& contrast_settings);
      }
    }
  }
}

