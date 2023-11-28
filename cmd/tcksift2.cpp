/* Copyright (c) 2008-2023 the MRtrix3 contributors.
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

#include "command.h"
#include "exception.h"
#include "image.h"
#include "header.h"

#include "file/path.h"

#include "math/sphere/set/adjacency.h"
#include "math/sphere/set/predefined.h"

#include "dwi/tractography/SIFT/sift.h"

#include "dwi/tractography/SIFT2/regularisation.h"
#include "dwi/tractography/SIFT2/tckfactor.h"
#include "dwi/tractography/SIFT2/types.h"





using namespace MR;
using namespace App;

using namespace MR::DWI;
using namespace MR::DWI::Tractography;
using namespace MR::DWI::Tractography::SIFT2;

using value_type = MR::DWI::Tractography::SIFT2::value_type;






const OptionGroup SIFT2IterationOption = OptionGroup ("Options for controlling iterations of the SIFT2 optimisation algorithm")

  + Option ("min_iters", "minimum number of iterations to run before testing for convergence; "
                         "this can prevent premature termination at early iterations if the cost function increases slightly "
                         "(default: " + str(SIFT2_MIN_ITERS_DEFAULT) + ")")
    + Argument ("count").type_integer (0)

  + Option ("max_iters", "maximum number of iterations to run before terminating program")
    + Argument ("count").type_integer (0)

  + Option ("min_cf_decrease", "minimum decrease in the cost function (as a fraction of the initial value) that must occur each iteration for the algorithm to continue "
                               "(default: " + str(SIFT2_MIN_CF_DECREASE_DEFAULT, 2) + ")")
    + Argument ("frac").type_float (0.0, 1.0);








const OptionGroup SIFT2InitOption = OptionGroup ("Options for initialising the SIFT2 model")

  + Option ("init_coeffs", "initialise the set of per-streamline coefficients for commencement of optimisation")
    + Argument ("file").type_file_in()

  + Option ("init_factors", "initialise the set of per-streamline weighting factors for commencement of optimisation")
    + Argument ("file").type_file_in()

  + Option ("in_coeffs", "provide the set of per-streamline coefficients; do not perform any subsequent optimisation of such")
    + Argument ("file").type_file_in()

  + Option ("in_factors", "provide the set of per-streamline weighting factors; do not perform any subsequent optimisation of such")
    + Argument ("file").type_file_in();





const OptionGroup SIFT2AbsoluteOption = OptionGroup ("Options for controlling SIFT2 optimisation in \"absolute\" mode")

  + Option ("min_td_frac", "minimum fraction of the FOD integral reconstructed by streamlines; "
                           "if the reconstructed streamline density is below this fraction, the fixel is excluded from optimisation "
                           "(default: " + str(SIFT2_MIN_TD_FRAC_DEFAULT, 2) + ")")
    + Argument ("fraction").type_float (0.0, 1.0)

  + Option ("min_factor", "minimum weighting factor for an individual streamline; "
                          "if the factor falls below this number the streamline will be rejected entirely (factor set to zero) "
                          "(default: " + str(std::exp (SIFT2_MIN_COEFF_DEFAULT), 2) + ")")
    + Argument ("factor").type_float (0.0, 1.0)

  + Option ("min_coeff", "minimum weighting coefficient for an individual streamline; "
                         "similar to the '-min_factor' option, but using the exponential coefficient basis of the SIFT2 model; "
                         "these parameters are related as: factor = e^(coeff). "
                         "Note that the -min_factor and -min_coeff options are mutually exclusive - you can only provide one. "
                         "(default: " + str(SIFT2_MIN_COEFF_DEFAULT, 2) + ")")
    + Argument ("coeff").type_float (-std::numeric_limits<default_type>::infinity(), 0.0)

  + Option ("max_factor", "maximum weighting factor that can be assigned to any one streamline "
                          "(default: " + str(std::exp (SIFT2_MAX_COEFF_DEFAULT), 2) + ")")
    + Argument ("factor").type_float (1.0)

  + Option ("max_coeff", "maximum weighting coefficient for an individual streamline; "
                         "similar to the '-max_factor' option, but using the exponential coefficient basis of the SIFT2 model; "
                         "these parameters are related as: factor = e^(coeff). "
                         "Note that the -max_factor and -max_coeff options are mutually exclusive - you can only provide one. "
                         "(default: " + str(SIFT2_MAX_COEFF_DEFAULT, 2) + ")")
    + Argument ("coeff").type_float (1.0)

  + Option ("max_coeff_step", "maximum change to a streamline's weighting coefficient in a single iteration "
                              "(default: " + str(SIFT2_MAX_COEFF_STEP_DEFAULT, 2) + ")")
    + Argument ("step").type_float (0.0)

  + Option ("linear", "perform a linear estimation of streamline weights, rather than the standard non-linear optimisation "
                      "(typically does not provide as accurate a model fit; but only requires a single pass)");






const OptionGroup SIFT2DiffOption = OptionGroup ("Options specific to operating SIFT2 in differential mode")

  + Option ("differential", "Estimate a set of differential weighting factors to fit fixel-wise FD differences")
    + Argument ("in_diff", "input fixel data file containing fibre density differences").type_image_in()
    + Argument ("out_delta", "output text file containing delta weight per streamline").type_file_out()

  + Option ("min_delta", "minimum delta weight for an individual streamline "
                         "(default: " + str(SIFT2_MIN_DELTA_DEFAULT, 2) + ")")
    + Argument ("delta").type_float (-std::numeric_limits<default_type>::infinity(), 0.0)

  + Option ("max_delta", "maximum delta weight for an individual streamline "
                         "(default: " + str(SIFT2_MAX_DELTA_DEFAULT, 2) + ")")
    + Argument ("delta").type_float (0.0, std::numeric_limits<default_type>::infinity())

  + Option ("max_delta_step", "maximum change to a streamline's delta weight in a single iteration "
                              "(default: " + str(SIFT2_MAX_DELTA_STEP_DEFAULT, 2) + ")")
    + Argument ("step").type_float (0.0);




void usage ()
{

  AUTHOR = "Robert E. Smith (robert.smith@florey.edu.au)";

  SYNOPSIS = "Optimise per-streamline cross-section multipliers to match a whole-brain tractogram to fixel-wise fibre densities";

  REFERENCES
    + "Smith, R. E.; Tournier, J.-D.; Calamante, F. & Connelly, A. " // Internal
    "SIFT2: Enabling dense quantitative assessment of brain white matter connectivity using streamlines tractography. "
    "NeuroImage, 2015, 119, 338-351"

    + "* If using the -linear option: \n"
    "Smith, RE; Raffelt, D; Tournier, J-D; Connelly, A. " // Internal
    "Quantitative Streamlines Tractography: Methods and Inter-Subject Normalisation. "
    "Open Science Framework, https://doi.org/10.31219/osf.io/c67kn.";

  ARGUMENTS
  + Argument ("in_tracks",   "the input track file").type_tracks_in()
  + Argument ("in_fd",       "input fixel data file containing fibre density estimates").type_image_in()
  + Argument ("out_weights", "output text file containing the weighting factor for each streamline").type_file_out();

  OPTIONS

  + SIFT::SIFTModelWeightsOption
  + SIFT::SIFTModelOption
  + SIFT::SIFTOutputOption

  + Option ("out_coeffs", "output text file containing the weighting coefficient for each streamline")
    + Argument ("path").type_file_out()

  + SIFT2IterationOption
  + SIFT2::RegularisationOptions
  + SIFT2InitOption
  + SIFT2AbsoluteOption
  + SIFT2DiffOption;

}




void run ()
{

  if (get_options("min_factor").size() && get_options("min_coeff").size())
    throw Exception ("Options -min_factor and -min_coeff are mutually exclusive");
  if (get_options("max_factor").size() && get_options("max_coeff").size())
    throw Exception ("Options -max_factor and -max_coeff are mutually exclusive");
  if (get_options ("linear").size() + get_options("init_coeffs").size() + get_options("init_factors").size() + get_options ("in_coeffs").size() + get_options("in_factors").size() > 1)
    throw Exception ("Options -linear, -init_coeffs, -init_factors, -in_coeffs and -in_factors are mutually exclusive");

  if (Path::has_suffix (argument[2], ".tck"))
    throw Exception ("Output of tcksift2 command should be a text file, not a tracks file");

  TckFactor tckfactor (argument[1]);

  if (App::get_options("fd_scale_gm").size())
    tckfactor.scale_FDs_by_GM();

  std::string debug_path = get_option_value<std::string> ("output_debug", "");
  if (debug_path.size())
    tckfactor.initialise_debug_image_output (debug_path);

  tckfactor.map_streamlines (argument[0]);
  tckfactor.store_orig_TDs();
  tckfactor.exclude_fixels (get_option_value ("min_td_frac", SIFT2_MIN_TD_FRAC_DEFAULT));
  tckfactor.calibrate_regularisation();

  if (debug_path.size()) {
    tckfactor.output_TD_images (debug_path, "origTD_fixel.mif", "trackcount_fixel.mif");
    tckfactor.output_all_debug_images (debug_path, "before");
  }

  auto opt = get_options ("out_mu");
  if (opt.size()) {
    File::OFStream out_mu (opt[0][0]);
    out_mu << tckfactor.mu();
  }

  opt = get_options ("csv");
  if (opt.size()) {
    if (!get_options ("differential").size() && (get_options ("linear").size() || get_options ("in_coeffs").size() || get_options ("in_factors").size())) {
      WARN ("-csv option will be ignored, as no iterative optimisation is taking place");
    } else {
      tckfactor.set_csv_path (opt[0][0]);
    }
  }

  if (get_options ("linear").size()) {

    tckfactor.calc_afcsa();

  } else if (get_options ("in_coeffs").size() || get_options ("in_factors").size()) {

    opt = get_options ("in_coeffs");
    if (opt.size()) {
      tckfactor.set_coefficients (opt[0][0]);
    } else {
      opt = get_options ("in_factors");
      assert (opt.size());
      tckfactor.set_factors (opt[0][0]);
    }

  } else {

    opt = get_options ("reg_basis_abs");
    if (opt.size())
      tckfactor.set_reg_basis_abs (reg_basis_t (int(opt[0][0])));
    opt = get_options ("reg_fn_abs");
    if (opt.size())
      tckfactor.set_reg_fn_abs (reg_fn_t (int(opt[0][0])));
    tckfactor.set_reg_lambda_abs (get_option_value("reg_strength_abs", SIFT2::regularisation_strength_abs_default));

    opt = get_options ("min_iters");
    if (opt.size())
      tckfactor.set_min_iters (int(opt[0][0]));
    opt = get_options ("max_iters");
    if (opt.size())
      tckfactor.set_max_iters (int(opt[0][0]));
    opt = get_options ("min_factor");
    if (opt.size())
      tckfactor.set_min_factor (value_type(opt[0][0]));
    opt = get_options ("min_coeff");
    if (opt.size())
      tckfactor.set_min_coeff (value_type(opt[0][0]));
    opt = get_options ("max_factor");
    if (opt.size())
      tckfactor.set_max_factor (value_type(opt[0][0]));
    opt = get_options ("max_coeff");
    if (opt.size())
      tckfactor.set_max_coeff (value_type(opt[0][0]));
    opt = get_options ("max_coeff_step");
    if (opt.size())
      tckfactor.set_max_coeff_step (value_type(opt[0][0]));
    opt = get_options ("min_cf_decrease");
    if (opt.size())
      tckfactor.set_min_cf_decrease (value_type(opt[0][0]));

    opt = get_options ("init_factors");
    if (opt.size())
      tckfactor.set_factors (opt[0][0]);
    opt = get_options ("init_coeffs");
    if (opt.size())
      tckfactor.set_coefficients (opt[0][0]);

    tckfactor.estimate_weights<operation_mode_t::ABSOLUTE>();
  }

  tckfactor.report_entropy();

  tckfactor.output_factors (argument[2]);

  opt = get_options ("out_coeffs");
  if (opt.size())
    tckfactor.output_coefficients (opt[0][0]);

  if (debug_path.size())
    tckfactor.output_all_debug_images (debug_path, "after");

  opt = get_options ("differential");
  if (opt.size()) {

    tckfactor.import_delta_data (opt[0][0]);
    const std::string output_delta_path = opt[0][1];
    tckfactor.set_reg_lambda_diff (get_option_value("reg_strength_diff", SIFT2::regularisation_strength_diff_default));

    if (debug_path.size())
      tckfactor.output_delta_debug_images (debug_path, "before");

    opt = get_options ("min_delta");
    if (opt.size())
      tckfactor.set_min_delta (value_type(opt[0][0]));
    opt = get_options ("max_delta");
    if (opt.size())
      tckfactor.set_max_delta (value_type(opt[0][0]));
    opt = get_options ("max_delta_step");
    if (opt.size())
      tckfactor.set_max_delta_step (value_type(opt[0][0]));

    tckfactor.estimate_weights<operation_mode_t::DIFFERENTIAL>();

    tckfactor.output_deltas (output_delta_path);
    if (debug_path.size())
      tckfactor.output_delta_debug_images (debug_path, "after");

  }

}


