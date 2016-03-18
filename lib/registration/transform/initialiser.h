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
        enum InitType {mass, geometric, moments, set_centre_mass, rot_search, none};
          // moments_unmasked, fod, mass_unmasked, moments_use_mask_intensity,
        struct LinearInitialisationParams {
          struct TranslationInit {
            bool unmasked1;
            bool unmasked2;
            TranslationInit () :
              unmasked1 (false),
              unmasked2 (false) {} // TODO config parsing
          };

          struct RotationInit {
            bool unmasked1;
            bool unmasked2;
            struct rot_search {
              std::vector<default_type> angles;
              default_type scale;
              size_t directions;
              bool run_global;
              struct global_search {
                size_t iterations;
                global_search () :
                  iterations (10000) {}
              };
              global_search global;
              rot_search () :
                angles (9),
                scale (0.1),
                directions (250),
                run_global (false) {
                  angles[0] =  2.0 / 180.0 * Math::pi;
                  angles[1] =  5.0 / 180.0 * Math::pi;
                  angles[2] = 10.0 / 180.0 * Math::pi;
                  angles[3] = 15.0 / 180.0 * Math::pi;
                  angles[4] = 20.0 / 180.0 * Math::pi;
                  angles[5] = 25.0 / 180.0 * Math::pi;
                  angles[6] = 30.0 / 180.0 * Math::pi;
                  angles[7] = 35.0 / 180.0 * Math::pi;
                  angles[8] = 40.0 / 180.0 * Math::pi;
                }
            };
            rot_search search;
            RotationInit () :
              unmasked1 (false),
              unmasked2 (false) {    } // TODO config parsing
          };
          TranslationInit init_translation;
          RotationInit init_rotation;
        };

        extern void set_centre_using_image_mass (
          Image<default_type>& im1,
          Image<default_type>& im2,
          Image<default_type>& mask1,
          Image<default_type>& mask2,
          Registration::Transform::Base& transform,
          Registration::Transform::Init::LinearInitialisationParams& init);

        extern void initialise_using_image_centres (
          const Image<default_type>& im1,
          const Image<default_type>& im2,
          Image<default_type>& mask1,
          Image<default_type>& mask2,
          Registration::Transform::Base& transform,
          Registration::Transform::Init::LinearInitialisationParams& init);

        extern void initialise_using_image_moments (
          Image<default_type>& im1,
          Image<default_type>& im2,
          Image<default_type>& mask1,
          Image<default_type>& mask2,
          Registration::Transform::Base& transform,
          Registration::Transform::Init::LinearInitialisationParams& init); // bool use_mask_values = false

        extern void initialise_using_FOD (
          Image<default_type>& im1,
          Image<default_type>& im2,
          Image<default_type>& mask1,
          Image<default_type>& mask2,
          Registration::Transform::Base& transform,
          Registration::Transform::Init::LinearInitialisationParams& init); // TODO ssize_t lmax = -1


        extern void initialise_using_rotation_search (
          Image<default_type>& im1,
          Image<default_type>& im2,
          Image<default_type>& mask1,
          Image<default_type>& mask2,
          Registration::Transform::Base& transform,
          Registration::Transform::Init::LinearInitialisationParams& init);

        extern void initialise_using_image_mass (
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

#endif
