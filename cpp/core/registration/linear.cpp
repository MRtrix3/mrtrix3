/* Copyright (c) 2008-2026 the MRtrix3 contributors.
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

#include <filesystem>

#include "enum.h"
#include "registration/linear.h"

namespace MR::Registration {

using namespace App;

// define parameters of initialisation methods used for both, rigid and affine registration
void parse_general_options(Registration::Linear &registration) {
  if (!get_options("init_translation.unmasked1").empty())
    registration.init.init_translation.unmasked1 = true;
  if (!get_options("init_translation.unmasked2").empty())
    registration.init.init_translation.unmasked2 = true;

  if (!get_options("init_rotation.unmasked1").empty())
    registration.init.init_rotation.unmasked1 = true;
  if (!get_options("init_rotation.unmasked2").empty())
    registration.init.init_rotation.unmasked2 = true;

  if (!get_options("init_rotation.search.run_global").empty())
    registration.init.init_rotation.search.run_global = true;
  auto opt = get_options("init_rotation.search.angles");
  if (!opt.empty()) {
    std::vector<default_type> angles = parse_floats(opt[0][0]);
    for (auto &a : angles) {
      if (a < 0.0 or a > 180.0)
        throw Exception("init_rotation.search.angles have to be between 0 and 180 degree.");
    }
    registration.init.init_rotation.search.angles.swap(angles);
  }
  opt = get_options("init_rotation.search.directions");
  if (!opt.empty()) {
    ssize_t dirs(opt[0][0]);
    if (dirs < 1)
      throw Exception("init_rotation.search.directions has to be at least 1");
    registration.init.init_rotation.search.directions = dirs;
  }
  opt = get_options("init_rotation.search.scale");
  if (!opt.empty()) {
    default_type scale = (opt[0][0]);
    if (scale < 0.0001 or scale > 1.0)
      throw Exception("init_rotation.search.scale has to be between 0.0001 and 1.0");
    registration.init.init_rotation.search.scale = scale;
  }
  opt = get_options("init_rotation.search.global.iterations");
  if (!opt.empty()) {
    size_t iters(opt[0][0]);
    if (iters == 0)
      throw Exception("init_rotation.search.global.iterations has to be at least 1");
    registration.init.init_rotation.search.global.iterations = iters;
  }

  opt = get_options("linstage.optimiser.default");
  if (!opt.empty())
    registration.set_stage_optimiser_default(MR::Enum::from_name<OptimiserAlgoType>(opt[0][0]));
  opt = get_options("linstage.optimiser.first");
  if (!opt.empty())
    registration.set_stage_optimiser_first(MR::Enum::from_name<OptimiserAlgoType>(opt[0][0]));
  opt = get_options("linstage.optimiser.last");
  if (!opt.empty())
    registration.set_stage_optimiser_last(MR::Enum::from_name<OptimiserAlgoType>(opt[0][0]));

  opt = get_options("linstage.iterations");
  if (!opt.empty()) {
    const auto iterations = parse_ints<uint32_t>(opt[0][0]);
    registration.set_stage_iterations(iterations);
  } else {
    registration.set_stage_iterations(std::vector<uint32_t>{1});
  }

  opt = get_options("linstage.diagnostics.dir");
  if (!opt.empty()) {
    const std::filesystem::path diag_dir{opt[0][0]};
    std::filesystem::create_directories(diag_dir);
    registration.set_diagnostics_image_dir(diag_dir);
  }
}

void set_init_translation_model_from_option(Registration::Linear &registration, const init_translation_t option) {
  switch (option) {
  case init_translation_t::mass:
    registration.set_init_translation_type(Registration::Transform::Init::mass);
    break;
  case init_translation_t::geometric:
    registration.set_init_translation_type(Registration::Transform::Init::geometric);
    break;
  case init_translation_t::none:
    registration.set_init_translation_type(Registration::Transform::Init::none);
    break;
  }
}

void set_init_rotation_model_from_option(Registration::Linear &registration, const init_rotation_t option) {
  switch (option) {
  //  TODO registration.set_init_type (Registration::Transform::Init::fod);
  case init_rotation_t::search:
    registration.set_init_rotation_type(Registration::Transform::Init::rot_search);
    break;
  case init_rotation_t::moments:
    registration.set_init_rotation_type(Registration::Transform::Init::moments);
    break;
  case init_rotation_t::none:
    registration.set_init_rotation_type(Registration::Transform::Init::none);
    break;
  }
}

// clang-format off
const OptionGroup adv_init_options =
    OptionGroup("Advanced linear transformation initialisation options")

    // translation options
    + Option("init_translation.unmasked1",
             "disregard mask1 for the translation initialisation"
             " (affects 'mass')")
    + Option("init_translation.unmasked2",
             "disregard mask2 for the translation initialisation"
             " (affects 'mass')")

    // rotation options
    + Option("init_rotation.unmasked1",
             "disregard mask1 for the rotation initialisation"
             " (affects 'search' and 'moments')")
    + Option("init_rotation.unmasked2",
             "disregard mask2 for the rotation initialisation"
             " (affects 'search' and 'moments')")
    + Option("init_rotation.search.angles",
             "rotation angles for the local search in degrees between 0 and 180.")
      + Argument("angles").type_sequence_float().set_default("2,5,10,15,20")
    + Option("init_rotation.search.scale",
             "relative size of the images used for the rotation search.")
      + Argument("scale").type_float(0.0001, 1.0).set_default("0.15")
    + Option("init_rotation.search.directions",
             "number of rotation axis for local search.")
      + Argument("num").type_integer(1, 10000).set_default(250)
    + Option("init_rotation.search.run_global",
             "perform a global rather than local initial rotation search.")
    + Option("init_rotation.search.global.iterations",
             "number of rotations to investigate")
      + Argument("num").type_integer(1, 1e10).set_default(10000);

const OptionGroup lin_stage_options =
    OptionGroup("Advanced linear registration stage options")

    + Option("linstage.iterations",
             "number of iterations for each registration stage."
             " Not to be confused with -rigid_niter or -affine_niter."
             " This can be used to generate intermediate diagnostics images"
             " (-linstage.diagnostics.dir)"
             " or to change the cost function optimiser during registration"
             " (without the need to repeatedly resize the images)."
             " (Default: 1 == no repetition)")
      + Argument("value(s)").type_sequence_int()

    // TODO linstage.loop_density

    // TODO linstage.robust: Start each stage repetition with the estimated parameters from the previous stage.
    // choose parameter consensus criterion: maximum overlap, min cost

    + Option("linstage.optimiser.first",
             "Cost function optimisation algorithm to use at first iteration of all stages."
             " BBGD (Barzilai-Borwein gradient descent);"
             " GD (simple gradient descent).")
      + Argument("algorithm").type_choice<OptimiserAlgoType>().set_default("bbgd")
    + Option("linstage.optimiser.last",
             "Cost function optimisation algorithm to use at last iteration of all stages"
             " (if there are more than one)."
             " BBGD (Barzilai-Borwein gradient descent);"
             " GD (simple gradient descent).")
      + Argument("algorithm").type_choice<OptimiserAlgoType>().set_default("bbgd")
    + Option("linstage.optimiser.default",
             "Cost function optimisation algorithm to use at any stage iteration"
             " other than first or last iteration."
             " BBGD (Barzilai-Borwein gradient descent);"
             " GD (simple gradient descent).")
      + Argument("algorithm").type_choice<OptimiserAlgoType>().set_default("bbgd")

    + Option("linstage.diagnostics.dir",
             "generate diagnostics images after every registration stage")
      + Argument("dir").type_directory_out(DirOutMode::MayExist);

const OptionGroup rigid_options =
    OptionGroup("Rigid registration options")

    + Option("rigid", "the output text file containing the rigid transformation as a 4x4 matrix")
      + Argument("file").type_file_out()

    + Option("rigid_1tomidway",
             "the output text file containing the rigid transformation"
             " that aligns image1 to image2 in their common midway space"
             " as a 4x4 matrix")
      + Argument("file").type_file_out()

    + Option("rigid_2tomidway",
             "the output text file containing the rigid transformation"
             " that aligns image2 to image1 in their common midway space"
             " as a 4x4 matrix")
      + Argument("file").type_file_out()

    + Option("rigid_init_translation",
             "initialise the translation and centre of rotation."
             " mass (aligns the centers of mass of both images);"
             " geometric (aligns geometric image centres);"
             " none.")
      + Argument("type").type_choice<init_translation_t>().set_default("mass")

    + Option("rigid_init_rotation",
             "Method to use to initialise the rotation."
             " search (search for the best rotation using mean squared residuals);" // TODO CC
             " moments (rotation based on directions of intensity variance with respect to centre of mass);"
             " none.") // TODO  This can be combined with rigid_init_translation.
      + Argument("type").type_choice<init_rotation_t>().set_default("none")

    + Option("rigid_init_matrix",
             "initialise either the rigid, affine, or syn registration"
             " with the supplied rigid transformation"
             " (as a 4x4 matrix in scanner coordinates)."
             "Note that this overrides rigid_init_translation and rigid_init_rotation initialisation") // TODO definition
                                                                                                       // of centre
      + Argument("file").type_file_in()

    + Option("rigid_scale",
             "use a multi-resolution scheme by defining a scale factor for each level"
             " using comma-separated values")
      + Argument("factor").type_sequence_float().set_default("0.25,0.5,1.0")

    + Option("rigid_niter",
             "the maximum number of gradient descent iterations per stage."
             " This can be specified either as a single number for all multi-resolution levels,"
             " or a single value for each level.")
      + Argument("num").type_sequence_int().set_default("1000")

    + Option("rigid_metric",
             "diff (intensity differences).")
             // " ncc (normalised cross-correlation);" TODO
      + Argument("type").type_choice<LinearMetricType>().set_default("diff")

    + Option("rigid_metric.diff.estimator",
             "Robust estimator to use during rigid-body registration."
             " l1 (least absolute: |x|);"
             " l2 (ordinary least squares);"
             " lp (least powers: |x|^1.2);"
             " none.")
      + Argument("type").type_choice<LinearRobustMetricEstimatorType>().set_default("none")

    // + Option ("rigid_loop_density", "density of gradient descent 1 (batch) to 0.0 (max stochastic) (Default: 1.0)")
    //   + Argument ("num").type_sequence_float () // TODO

    // + Option ("rigid_repetitions", " ")
    //   + Argument ("num").type_sequence_int () // TODO

    + Option("rigid_lmax",
             "explicitly set the lmax to be used per scale factor in rigid FOD registration."
             " By default, FOD registration will use lmax 0,2,4"
             " with default scale factors 0.25,0.5,1.0 respectively."
             " Note that no reorientation will be performed with lmax = 0.")
      + Argument("num").type_sequence_lmax()

    + Option("rigid_log",
             "write gradient descent parameter evolution to log file")
      + Argument("file").type_file_out();

const OptionGroup affine_options =
    OptionGroup("Affine registration options")

    + Option("affine",
             "the output text file containing the affine transformation as a 4x4 matrix")
      + Argument("file").type_file_out()

    + Option("affine_1tomidway",
             "the output text file containing the affine transformation"
             " that aligns image1 to image2 in their common midway space"
             " as a 4x4 matrix")
      + Argument("file").type_file_out()

    + Option("affine_2tomidway",
             "the output text file containing the affine transformation"
             " that aligns image2 to image1 in their common midway space"
             " as a 4x4 matrix")
      + Argument("file").type_file_out()

    + Option("affine_init_translation",
             "initialise the translation and centre of rotation."
             " mass (aligns the centers of mass of both images);"
             " geometric (aligns geometric image centres);"
             " none.")
      + Argument("type").type_choice<init_translation_t>().set_default("mass")

    + Option("affine_init_rotation",
             "initialise the rotation."
             " search (search for the best rotation using mean squared residuals);"
             " moments (rotation based on directions of intensity variance with respect to centre of mass);"
             " none.") // TODO  This can be combined with affine_init_translation.
      + Argument("type").type_choice<init_rotation_t>().set_default("none")

    + Option("affine_init_matrix",
             "initialise either the affine or syn registration"
             " with the supplied affine transformation"
             " (as a 4x4 matrix in scanner coordinates)."
             " Note that this overrides affine_init_translation and affine_init_rotation initialisation") // TODO definition
      + Argument("file").type_file_in()                                                                   // of centre

    + Option("affine_scale",
             "use a multi-resolution scheme"
             " by defining a scale factor for each level using comma separated values")
      + Argument("factor").type_sequence_float().set_default("0.25,0.5,1.0")

    + Option("affine_niter",
             "the maximum number of gradient descent iterations per stage."
             " This can be specified either as a single number for all multi-resolution levels,"
             " or a single value for each level.")
      + Argument("num").type_sequence_int().set_default("1000")

    + Option("affine_metric",
             "diff (intensity differences).")
             // "ncc (normalised cross-correlation) " TODO
      + Argument("type").type_choice<LinearMetricType>().set_default("diff")

    + Option("affine_metric.diff.estimator",
             "Robust estimator to use durring affine registration."
             " l1 (least absolute: |x|);"
             " l2 (ordinary least squares);"
             " lp (least powers: |x|^1.2);"
             " none.")
      + Argument("type").type_choice<LinearRobustMetricEstimatorType>().set_default("none")

    // + Option ("affine_loop_density", "density of gradient descent 1 (batch) to 0.0 (max stochastic) (Default: 1.0)")
    //   + Argument ("num").type_sequence_float () // TODO

    // + Option ("affine_repetitions", " ")
    //   + Argument ("num").type_sequence_int () // TODO

    + Option("affine_lmax",
             "explicitly set the lmax to be used per scale factor in affine FOD registration."
             " By default FOD registration will use lmax 0,2,4"
             " with default scale factors 0.25,0.5,1.0 respectively."
             " Note that no reorientation will be performed with lmax = 0.")
      + Argument("num").type_sequence_lmax()

    + Option("affine_log",
             "write gradient descent parameter evolution to log file")
      + Argument("file").type_file_out();

const OptionGroup fod_options =
    OptionGroup("FOD registration options")

    + Option("directions",
             "file containing the directions used for FOD reorientation using apodised point spread functions")
      + Argument("file").type_file_in().set_default("built-in 60-direction set")

    + Option("noreorientation",
             "turn off FOD reorientation."
             " Reorientation is on by default if the number of volumes in the 4th dimension"
             " corresponds to the number of coefficients in an antipodally symmetric spherical harmonic series"
             " (i.e. 6, 15, 28, 45, 66 etc)");
// clang-format on

} // namespace MR::Registration
