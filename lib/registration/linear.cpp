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

#include "registration/linear.h"

namespace MR
{
  namespace Registration
  {

    using namespace App;

    const char* initialisation_choices[] = { "mass", "geometric", "moments", "mass_unmasked", "moments_use_mask_intensity", "moments_unmasked", "fod", "none", NULL };
    const char* linear_metric_choices[] = { "diff", "ncc", NULL };
    const char* linear_robust_estimator_choices[] = { "l1", "l2", "lp", NULL };

    void set_init_model_from_option (Registration::Linear& registration, const int& option) {
      switch (option){
        case 0:
          registration.set_init_type (Registration::Transform::Init::mass);
          break;
        case 1:
          registration.set_init_type (Registration::Transform::Init::geometric);
          break;
        case 2:
          registration.set_init_type (Registration::Transform::Init::moments);
          break;
        case 3:
          registration.set_init_type (Registration::Transform::Init::mass_unmasked);
          break;
        case 4:
          registration.set_init_type (Registration::Transform::Init::moments_use_mask_intensity);
          break;
        case 5:
          registration.set_init_type (Registration::Transform::Init::moments_unmasked);
          break;
        case 6:
          registration.set_init_type (Registration::Transform::Init::fod);
          break;
        case 7:
          registration.set_init_type (Registration::Transform::Init::none);
          break;
        default:
          break;
      }
    }

    const OptionGroup rigid_options =
      OptionGroup ("Rigid registration options")

      + Option ("rigid", "the output text file containing the rigid transformation as a 4x4 matrix")
        + Argument ("file").type_file_out ()

      + Option ("rigid_centre", "initialise the centre of rotation and initial translation. "
                                "Valid choices are: mass (which uses the image center of mass), "
                                "geometric (geometric image centre), moments (image moments), "
                                "mass_unmasked (don't use image masks for centre of mass initialisation), "
                                "moments_unmasked (don't use image masks for moments initialisation), "
                                "fod (aligns FOD images based on their centre of mass and global sum of all fibre orientations) "
                                "or none."
                                "Default: mass.")
        + Argument ("type").type_choice (initialisation_choices)

      + Option ("rigid_init", "initialise either the rigid, affine, or syn registration with the supplied rigid transformation (as a 4x4 matrix). Note that this overrides rigid_centre initialisation")
        + Argument ("file").type_file_in ()

      + Option ("rigid_scale", "use a multi-resolution scheme by defining a scale factor for each level "
                               "using comma separated values (Default: 0.5,1)")
        + Argument ("factor").type_sequence_float ()

      + Option ("rigid_niter", "the maximum number of iterations. This can be specified either as a single number "
                               "for all multi-resolution levels, or a single value for each level. (Default: 1000)")
        + Argument ("num").type_sequence_int ()

      + Option ("rigid_metric", "valid choices are: l2 (ordinary least squares), "
                                "lp (least powers: |x|^1.2), "
                                "ncc (normalised cross-correlation) "
                                "Default: ordinary least squares")
        + Argument ("type").type_choice (linear_metric_choices)

      + Option ("rigid_global_search", "perform global search for most promising starting point. default: false")

      + Option ("rigid_lmax", "explicitly set the lmax to be used per scale factor in rigid FOD registration. By default FOD registration will "
                              "use lmax 0,2,4 with default scale factors 0.25,0.5,1.0 respectively. Note that no reorientation will be performed with lmax = 0.")
      + Argument ("num").type_sequence_int ();


    const OptionGroup affine_options =
        OptionGroup ("Affine registration options")

      + Option ("affine", "the output text file containing the affine transformation that aligns "
        "input image 1 to input image 2 as a 4x4 matrix")
        + Argument ("file").type_file_out ()

      + Option ("affine_2tomidway", "the output text file containing the affine transformation that aligns "
        "image2 to image1 in their common midway space as a 4x4 matrix")
        + Argument ("file").type_file_out ()

      + Option ("affine_1tomidway", "the output text file containing the affine transformation that "
        "aligns image1 to image2 in their common midway space as a 4x4 matrix")
        + Argument ("file").type_file_out ()

      + Option ("affine_centre", "initialise the centre of rotation and initial translation. "
                                "Valid choices are: mass (which uses the image center of mass), "
                                "geometric (geometric image centre), moments (image moments), "
                                "mass_unmasked (don't use image masks for centre of mass initialisation), "
                                "moments_unmasked (don't use image masks for moments initialisation), "
                                "fod (aligns FOD images based on their centre of mass and global sum of all fibre orientations) "
                                "or none."
                                "Default: mass.")
        + Argument ("type").type_choice (initialisation_choices)

      + Option ("affine_init", "initialise either the affine, or syn registration with the supplied affine transformation (as a 4x4 matrix). Note that this overrides affine_centre initialisation")
        + Argument ("file").type_file_in ()

      + Option ("affine_scale", "use a multi-resolution scheme by defining a scale factor for each level "
                               "using comma separated values (Default: 0.25,0.5,1.0)")
        + Argument ("factor").type_sequence_float ()

      + Option ("affine_niter", "the maximum number of iterations. This can be specified either as a single number "
                                "for all multi-resolution levels, or a single value for each level. (Default: 1000)")
        + Argument ("num").type_sequence_int ()

      + Option ("affine_loop_density", "density of gradient descent 1 (batch) to 0.0 (max stochastic) (Default: 1.0)")
        + Argument ("num").type_sequence_float ()

      + Option ("affine_repetitions", "number of repetitions with identical settings for each scale level")
        + Argument ("num").type_sequence_int ()

      + Option ("affine_metric", "valid choices are: "
                                 "diff (intensity differences), "
                                 "ncc (normalised cross-correlation) "
                                 "Default: diff")
        + Argument ("type").type_choice (linear_metric_choices)

      + Option ("affine_robust_estimator", "Valid choices are: "
                                  "l1 (least absolute: |x|), "
                                  "l2 (ordinary least squares), "
                                  "lp (least powers: |x|^1.2), "
                                  "Default: l2")
        + Argument ("type").type_choice (linear_robust_estimator_choices)

      + Option ("affine_robust_median", "use robust median estimator. default: false")

      + Option ("affine_global_search", "perform global search for most promising starting point. default: false")

      + Option ("affine_lmax", "explicitly set the lmax to be used per scale factor in affine FOD registration. By default FOD registration will "
                               "use lmax 0,2,4 with default scale factors 0.25,0.5,1.0 respectively. Note that no reorientation will be performed with lmax = 0.")
      + Argument ("num").type_sequence_int ();



    const OptionGroup fod_options =
      OptionGroup ("FOD registration options")

      + Option ("directions", "the directions used for FOD reorienation using apodised point spread functions (Default: 60 directions)")
      + Argument ("file", "a list of directions [az el] generated using the gendir command.").type_file_in ()

      + Option ("noreorientation", "turn off FOD reorientation. Reorientation is on by default if the number "
                                   "of volumes in the 4th dimension corresponds to the number of coefficients in an "
                                   "antipodally symmetric spherical harmonic series (i.e. 6, 15, 28, 45, 66 etc");

  }
}

