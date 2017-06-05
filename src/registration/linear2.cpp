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


#include "registration/linear2.h"

namespace MR
{
  namespace Registration
  {

    using namespace App;

    const char* linear_metric_choices[] = { "diff", "ncc", nullptr };
    const char* linear_robust_estimator_choices[] = { "l1", "l2", "lp", nullptr };
    const char* linear_optimisation_algo_choices[] = { "bbgd", "gd", nullptr };
    const char* optim_algo_names[] = { "BBGD", "GD", nullptr };

    // define parameters of initialisation methods used for both, rigid and affine registration
    void parse_general_options (Registration::Linear& registration) {
      auto opt = get_options("linstage.optimiser.default");
      if (opt.size()) {
        switch ((int) opt[0][0]) {
        case 0:
          registration.set_stage_optimiser_default (Registration::OptimiserAlgoType::bbgd);
          break;
        case 1:
          registration.set_stage_optimiser_default (Registration::OptimiserAlgoType::gd);
          break;
        }
      }

      opt = get_options("linstage.optimiser.first");
      if (opt.size()) {
        switch ((int) opt[0][0]) {
        case 0:
          registration.set_stage_optimiser_first (Registration::OptimiserAlgoType::bbgd);
          break;
        case 1:
          registration.set_stage_optimiser_first (Registration::OptimiserAlgoType::gd);
          break;
        }
      }

      opt = get_options("linstage.optimiser.last");
      if (opt.size()) {
        switch ((int) opt[0][0]) {
        case 0:
          registration.set_stage_optimiser_last (Registration::OptimiserAlgoType::bbgd);
          break;
        case 1:
          registration.set_stage_optimiser_last (Registration::OptimiserAlgoType::gd);
          break;
        }
      }

      opt = get_options("linstage.iterations");
      if (opt.size()) {
        vector<int> iterations = parse_ints (opt[0][0]);
        registration.set_stage_iterations (iterations);
      }

      opt = get_options("linstage.diagnostics.prefix");
      if (opt.size()) {
        registration.set_diagnostics_image_prefix (opt[0][0]);
      }
    }

    const OptionGroup lin_stage_options =
      OptionGroup ("Advanced linear registration stage options")
      + Option ("linstage.iterations", "number of iterations for each registration stage, not to be confused with -rigid_niter or -affine_niter. "
        "This can be used to generate intermediate diagnostics images (-linstage.diagnostics.prefix) "
        "or to change the cost function optimiser during registration (without the need to repeatedly resize the images). (Default: 1 == no repetition)")
        + Argument ("num or comma separated list").type_sequence_int ()

      // TODO linstage.loop_density

      // TODO linstage.robust: Start each stage repetition with the estimated parameters from the previous stage.
      // choose parameter consensus criterion: maximum overlap, min cost

      + Option ("linstage.optimiser.first", "Cost function optimisation algorithm to use at first iteration of all stages. "
        "Valid choices: bbgd (Barzilai-Borwein gradient descent) or gd (simple gradient descent). (Default: bbgd)")
        + Argument ("algorithm").type_choice (linear_optimisation_algo_choices)
      + Option ("linstage.optimiser.last", "Cost function optimisation algorithm to use at last iteration of all stages (if there are more than one). "
        "Valid choices: bbgd (Barzilai-Borwein gradient descent) or gd (simple gradient descent). (Default: bbgd)")
        + Argument ("algorithm").type_choice (linear_optimisation_algo_choices)
      + Option ("linstage.optimiser.default", "Cost function optimisation algorithm to use at any stage iteration other than first or last iteration. "
        "Valid choices: bbgd (Barzilai-Borwein gradient descent) or gd (simple gradient descent). (Default: bbgd)")
        + Argument ("algorithm").type_choice (linear_optimisation_algo_choices)

      + Option ("linstage.diagnostics.prefix", "generate diagnostics images after every registration stage")
        + Argument ("file prefix").type_text();

    const OptionGroup rigid_options =
      OptionGroup ("Rigid registration options")

      + Option ("rigid", "the output text file containing the rigid transformation as a 4x4 matrix")
        + Argument ("file").type_file_out ()

      + Option ("rigid_1tomidway", "the output text file containing the rigid transformation that "
        "aligns image1 to image2 in their common midway space as a 4x4 matrix")
        + Argument ("file").type_file_out ()

      + Option ("rigid_2tomidway", "the output text file containing the rigid transformation that aligns "
        "image2 to image1 in their common midway space as a 4x4 matrix")
        + Argument ("file").type_file_out ()


      + Option ("rigid_init_matrix", "initialise either the rigid, affine, or syn registration with "
                                "the supplied rigid transformation (as a 4x4 matrix in scanner coordinates). "
                                "Note that this overrides rigid_init_translation and rigid_init_rotation initialisation ") // TODO definition of centre
        + Argument ("file").type_file_in ()

      + Option ("rigid_scale", "use a multi-resolution scheme by defining a scale factor for each level "
                               "using comma separated values (Default: 0.25,0.5,1.0)")
        + Argument ("factor").type_sequence_float ()

      + Option ("rigid_niter", "the maximum number of gradient descent iterations per stage. This can be specified either as a single number "
                               "for all multi-resolution levels, or a single value for each level. (Default: 1000)")
        + Argument ("num").type_sequence_int ()

      + Option ("rigid_metric", "valid choices are: "
                                 "diff (intensity differences), "
                                 // "ncc (normalised cross-correlation) " TODO
                                 "Default: diff")
        + Argument ("type").type_choice (linear_metric_choices)

      + Option ("rigid_metric.diff.estimator", "Valid choices are: "
                                  "l1 (least absolute: |x|), "
                                  "l2 (ordinary least squares), "
                                  "lp (least powers: |x|^1.2), "
                                  "Default: l2")
        + Argument ("type").type_choice (linear_robust_estimator_choices)

      // + Option ("rigid_loop_density", "density of gradient descent 1 (batch) to 0.0 (max stochastic) (Default: 1.0)")
      //   + Argument ("num").type_sequence_float () // TODO

      // + Option ("rigid_repetitions", " ")
      //   + Argument ("num").type_sequence_int () // TODO

      + Option ("rigid_lmax", "explicitly set the lmax to be used per scale factor in rigid FOD registration. By default FOD registration will "
                              "use lmax 0,2,4 with default scale factors 0.25,0.5,1.0 respectively. Note that no reorientation will be performed with lmax = 0.")
      + Argument ("num").type_sequence_int ()

      + Option ("rigid_log", "write gradient descent parameter evolution to log file")
      + Argument ("file").type_file_out ();


    const OptionGroup affine_options =
        OptionGroup ("Affine registration options")

      + Option ("affine", "the output text file containing the affine transformation as a 4x4 matrix")
        + Argument ("file").type_file_out ()

      + Option ("affine_1tomidway", "the output text file containing the affine transformation that "
        "aligns image1 to image2 in their common midway space as a 4x4 matrix")
        + Argument ("file").type_file_out ()

      + Option ("affine_2tomidway", "the output text file containing the affine transformation that aligns "
        "image2 to image1 in their common midway space as a 4x4 matrix")
        + Argument ("file").type_file_out ()

      + Option ("affine_init_matrix", "initialise either the affine, or syn registration with "
                                "the supplied affine transformation (as a 4x4 matrix in scanner coordinates). "
                                "Note that this overrides affine_init_translation and affine_init_rotation initialisation ") // TODO definition of centre
        + Argument ("file").type_file_in ()

      + Option ("affine_scale", "use a multi-resolution scheme by defining a scale factor for each level "
                               "using comma separated values (Default: 0.25,0.5,1.0)")
        + Argument ("factor").type_sequence_float ()

      + Option ("affine_niter", "the maximum number of gradient descent iterations per stage. This can be specified either as a single number "
                               "for all multi-resolution levels, or a single value for each level. (Default: 1000)")
        + Argument ("num").type_sequence_int ()

      + Option ("affine_metric", "valid choices are: "
                                 "diff (intensity differences), "
                                 // "ncc (normalised cross-correlation) " TODO
                                 "Default: diff")
        + Argument ("type").type_choice (linear_metric_choices)

      + Option ("affine_metric.diff.estimator", "Valid choices are: "
                                  "l1 (least absolute: |x|), "
                                  "l2 (ordinary least squares), "
                                  "lp (least powers: |x|^1.2), "
                                  "Default: l2")
        + Argument ("type").type_choice (linear_robust_estimator_choices)

      // + Option ("affine_loop_density", "density of gradient descent 1 (batch) to 0.0 (max stochastic) (Default: 1.0)")
      //   + Argument ("num").type_sequence_float () // TODO

      // + Option ("affine_repetitions", " ")
      //   + Argument ("num").type_sequence_int () // TODO

      + Option ("affine_lmax", "explicitly set the lmax to be used per scale factor in affine FOD registration. By default FOD registration will "
                              "use lmax 0,2,4 with default scale factors 0.25,0.5,1.0 respectively. Note that no reorientation will be performed with lmax = 0.")
      + Argument ("num").type_sequence_int ()

      + Option ("affine_log", "write gradient descent parameter evolution to log file")
      + Argument ("file").type_file_out ();

  }
}

