/* Copyright (c) 2008-2024 the MRtrix3 contributors.
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
#include "header.h"
#include "image.h"

#include "file/config.h"
#include "file/path.h"

#include "dwi/directions/set.h"

#include "dwi/tractography/mapping/fixel_td_map.h"

#include "dwi/tractography/SIFT/proc_mask.h"
#include "dwi/tractography/SIFT/sift.h"

#include "dwi/tractography/SIFT2/tckfactor.h"
#include "dwi/tractography/SIFT2/units.h"

using namespace MR;
using namespace App;

using namespace MR::DWI;
using namespace MR::DWI::Tractography;
using namespace MR::DWI::Tractography::SIFT2;

// clang-format off
const OptionGroup SIFT2RegularisationOption = OptionGroup ("Regularisation options for SIFT2")
  + Option ("reg_tikhonov", "provide coefficient for regularising streamline weighting coefficients"
                            " (Tikhonov regularisation)"
                            " (default: " + str(SIFT2_REGULARISATION_TIKHONOV_DEFAULT, 2) + ")")
    + Argument ("value").type_float(0.0)
  + Option ("reg_tv", "provide coefficient for regularising variance of streamline weighting coefficient"
                      " to fixels along its length"
                      " (Total Variation regularisation)"
                      " (default: " + str(SIFT2_REGULARISATION_TV_DEFAULT, 2) + ")")
    + Argument ("value").type_float(0.0);

const OptionGroup SIFT2AlgorithmOption = OptionGroup ("Options for controlling the SIFT2 optimisation algorithm")
  + Option ("min_td_frac", "minimum fraction of the FOD integral reconstructed by streamlines;"
                           " if the reconstructed streamline density is below this fraction,"
                           " the fixel is excluded from optimisation"
                           " (default: " + str(SIFT2_MIN_TD_FRAC_DEFAULT, 2) + ")")
    + Argument ("fraction").type_float(0.0, 1.0)
  + Option ("min_iters", "minimum number of iterations to run before testing for convergence;"
                         " this can prevent premature termination at early iterations"
                         " if the cost function increases slightly"
                         " (default: " + str(SIFT2_MIN_ITERS_DEFAULT) + ")")
    + Argument ("count").type_integer(0)
  + Option ("max_iters", "maximum number of iterations to run before terminating program")
    + Argument ("count").type_integer(0)
  + Option ("min_factor", "minimum weighting factor for an individual streamline;"
                          " if the factor falls below this number,"
                          " the streamline will be rejected entirely"
                          " (factor set to zero)"
                          " (default: " + str(std::exp (SIFT2_MIN_COEFF_DEFAULT), 2) + ")")
    + Argument ("factor").type_float(0.0, 1.0)
  + Option ("min_coeff", "minimum weighting coefficient for an individual streamline;"
                         " similar to the '-min_factor' option,"
                         " but using the exponential coefficient basis of the SIFT2 model;"
                         " these parameters are related as:"
                         " factor = e^(coeff)."
                         " Note that the -min_factor and -min_coeff options are mutually exclusive;"
                         " you can only provide one."
                         " (default: " + str(SIFT2_MIN_COEFF_DEFAULT, 2) + ")")
    + Argument ("coeff").type_float(-std::numeric_limits<default_type>::infinity(), 0.0)
  + Option ("max_factor", "maximum weighting factor that can be assigned to any one streamline"
                          " (default: " + str(std::exp (SIFT2_MAX_COEFF_DEFAULT), 2) + ")")
    + Argument ("factor").type_float(1.0)
  + Option ("max_coeff", "maximum weighting coefficient for an individual streamline;"
                         " similar to the '-max_factor' option,"
                         " but using the exponential coefficient basis of the SIFT2 model;"
                         " these parameters are related as:"
                         " factor = e^(coeff)."
                         " Note that the -max_factor and -max_coeff options are mutually exclusive;"
                         " you can only provide one."
                         " (default: " + str(SIFT2_MAX_COEFF_DEFAULT, 2) + ")")
    + Argument ("coeff").type_float(1.0)
  + Option ("max_coeff_step", "maximum change to a streamline's weighting coefficient in a single iteration"
                              " (default: " + str(SIFT2_MAX_COEFF_STEP_DEFAULT, 2) + ")")
    + Argument ("step").type_float()
  + Option ("min_cf_decrease", "minimum decrease in the cost function"
                               " (as a fraction of the initial value)"
                               " that must occur each iteration for the algorithm to continue"
                               " (default: " + str(SIFT2_MIN_CF_DECREASE_DEFAULT, 2) + ")")
    + Argument ("frac").type_float(0.0, 1.0)
  + Option ("linear", "perform a linear estimation of streamline weights,"
                      " rather than the standard non-linear optimisation"
                      " (typically does not provide as accurate a model fit;"
                      " but only requires a single pass)");

void usage() {

  AUTHOR = "Robert E. Smith (robert.smith@florey.edu.au)";

  SYNOPSIS = "Optimise per-streamline cross-section multipliers"
             " to match a whole-brain tractogram to fixel-wise fibre densities";

  DESCRIPTION
    + "Interpretation of not just the relative magnitudes of the output weights of different streamlines,"
      " but their ABSOLUTE magnitude,"
      " depends on the presence or absence of any modulations applied to those values;"
      " by the tcksift2 command itself,"
      " and/or other experimental factors applied,"
      " whether implicit or explicit."
      " This has been termed \"inter-subject connection density normalisation\"."
      " Within the scope of the tcksift2 command,"
      " some control of this normalisation is available by specifying the units of those output weights."
      " The options available for these units,"
      " and their corresponding interpretations,"
      " are described in further detail in the following paragraphs."

    + "- \"NOS\" (Number Of Streamlines) / \"none\":"
      " No explicit scaling of the output streamline weights is performed."
      " A key component of the SIFT model as originally devised"
      " was to scale the contributions of all streamlines by proportionality coefficient mu,"
      " to facilitate direct comparison of tractogram and fixel-wise fibre densities."
      " This is therefore the \"native\" form in which these streamline weights are computed."
      " In the contex of output of the SIFT2 method,"
      " this makes the per-streamline weights approximately centred around unity,"
      " such that the overall magnitude of inter-areal connection weights"
      " will be comparable to that of the number-of-streamlines metric."
      " This was the behaviour of the tcksift2 command prior to software version 3.1.0."

    + "- \"AFD/mm\" / \"AFD.mm-1\", \"AFD.mm^-1\":"
      " The streamline weights in their native representation"
      " are multiplied by SIFT model proportionality coefficient mu"
      " as they are exported to file."
      " These values encode the AFD per millimetre of length"
      " that is contributed to the model by that streamline."
      " Only under specific circumstances"
      " does utilising these units permit direct comparison of Fibre Bundle Capacity (FBC)"
      " between reconstructions:"
      " a) Use of common response function(s);"
      " b) Having used some mechanism for global intensity normalisation"
      " (as required for any analysis of AFD);"
      " c) All DWI data have the same spatial resolution."

    + "- \"mm2\" / \"mm^2\":"
      " The streamline weights in their native representation"
      " are multiplied both by SIFT model proportionality coefficient mu"
      " and by the voxel volume in mm^3 as they are exported to file."
      " These units interpret the fixel-wise AFD values as volume fractions"
      " (despite the fact that these values do not have an upper bound of 1.0),"
      " such that the streamline weights may be interpreted"
      " as a physical fibre cross-sectional area in units of mm^2;"
      " each streamline therefore contributes some fibre volume per unit length."
      " Only under specific circumstances"
      " does utilising these units permit direct comparison of Fibre Bundle Capacity (FBC)"
      " between reconstructions:"
      " a) Use of common response function(s);"
      " b) Having used some mechanism for global intensity normalisation"
      " (as required for any analysis of AFD)."
      " Unlike the AFD/mm units however,"
      " streamline weights exported in these units are invariant"
      " to the resolution of the FOD voxel grid used in the SIFT2 optimisation.";

  REFERENCES
    + "Smith, R. E.; Tournier, J.-D.; Calamante, F. & Connelly, A. " // Internal
    "SIFT2: Enabling dense quantitative assessment of brain white matter connectivity"
    " using streamlines tractography. "
    "NeuroImage, 2015, 119, 338-351"

    + "Smith, RE; Raffelt, D; Tournier, J-D; Connelly, A. " // Internal
    "Quantitative Streamlines Tractography:"
    " Methods and Inter-Subject Normalisation. "
    "OHBM Aperture, doi: 10.52294/ApertureNeuro.2022.2.NEOD9565.";

  ARGUMENTS
  + Argument ("in_tracks", "the input track file").type_tracks_in()
  + Argument ("in_fod", "input image containing the spherical harmonics of the fibre orientation distributions").type_image_in()
  + Argument ("out_weights", "output text file containing the weighting factor for each streamline").type_file_out();

  OPTIONS
  + Option ("units", "specify the physical units for the output streamline weights (see Description)")
     + Argument ("choice").type_choice(SIFT2::units_choices)

  + SIFT::SIFTModelProcMaskOption
  + SIFT::SIFTModelOption
  + SIFT::SIFTOutputOption

  + Option ("out_coeffs", "output text file containing the weighting coefficient for each streamline")
    + Argument ("path").type_file_out()

  + SIFT2RegularisationOption
  + SIFT2AlgorithmOption;

}
// clang-format on

// CONF option: SIFT2DefaultUnits
// CONF default: "mm^2"
// CONF A string indicating the units of the streamline weights
// CONF yielded by the tcksift2 command.
SIFT2::units_t get_units() {
  auto opt = get_options("units");
  if (!opt.empty()) {
    try {
      return SIFT2::str2units(opt[0][0]);
    } catch (Exception& e) {
      throw Exception("Incorrectly specified SIFT2 units on command-line");
    }
  }
  const std::string from_config = File::Config::get("SIFT2DefaultUnits");
  if (from_config.empty())
    return SIFT2::default_units;
  try {
    return SIFT2::str2units(from_config);
  } catch (Exception& e) {
    throw Exception(e, "Incorrectly specified SIFT2 units in MRtrix config file");
  }
}

void run() {

  if (!get_options("min_factor").empty() && !get_options("min_coeff").empty())
    throw Exception("Options -min_factor and -min_coeff are mutually exclusive");
  if (!get_options("max_factor").empty() && !get_options("max_coeff").empty())
    throw Exception("Options -max_factor and -max_coeff are mutually exclusive");

  if (Path::has_suffix(argument[2], ".tck"))
    throw Exception("Output of tcksift2 command should be a text file, not a tracks file");

  const SIFT2::units_t units = get_units();

  auto in_dwi = Image<float>::open(argument[1]);

  DWI::Directions::FastLookupSet dirs(1281);

  TckFactor tckfactor(in_dwi, dirs);

  tckfactor.perform_FOD_segmentation(in_dwi);
  tckfactor.scale_FDs_by_GM();

  std::string debug_path = get_option_value<std::string>("output_debug", "");
  if (!debug_path.empty()) {
    tckfactor.initialise_debug_image_output(debug_path);
    tckfactor.output_proc_mask(Path::join(debug_path, "proc_mask.mif"));
  }

  tckfactor.map_streamlines(argument[0]);
  tckfactor.store_orig_TDs();

  tckfactor.remove_excluded_fixels(get_option_value("min_td_frac", SIFT2_MIN_TD_FRAC_DEFAULT));

  if (!debug_path.empty()) {
    tckfactor.output_TD_images(debug_path, "origTD_fixel.mif", "trackcount_fixel.mif");
    tckfactor.output_all_debug_images(debug_path, "before");
  }

  if (!get_options("linear").empty()) {

    tckfactor.calc_afcsa();

  } else {

    auto opt = get_options("csv");
    if (!opt.empty())
      tckfactor.set_csv_path(opt[0][0]);

    const float reg_tikhonov = get_option_value("reg_tikhonov", SIFT2_REGULARISATION_TIKHONOV_DEFAULT);
    const float reg_tv = get_option_value("reg_tv", SIFT2_REGULARISATION_TV_DEFAULT);
    tckfactor.set_reg_lambdas(reg_tikhonov, reg_tv);

    opt = get_options("min_iters");
    if (!opt.empty())
      tckfactor.set_min_iters(int(opt[0][0]));
    opt = get_options("max_iters");
    if (!opt.empty())
      tckfactor.set_max_iters(int(opt[0][0]));
    opt = get_options("min_factor");
    if (!opt.empty())
      tckfactor.set_min_factor(float(opt[0][0]));
    opt = get_options("min_coeff");
    if (!opt.empty())
      tckfactor.set_min_coeff(float(opt[0][0]));
    opt = get_options("max_factor");
    if (!opt.empty())
      tckfactor.set_max_factor(float(opt[0][0]));
    opt = get_options("max_coeff");
    if (!opt.empty())
      tckfactor.set_max_coeff(float(opt[0][0]));
    opt = get_options("max_coeff_step");
    if (!opt.empty())
      tckfactor.set_max_coeff_step(float(opt[0][0]));
    opt = get_options("min_cf_decrease");
    if (!opt.empty())
      tckfactor.set_min_cf_decrease(float(opt[0][0]));

    tckfactor.estimate_factors();
  }

  tckfactor.report_entropy();

  tckfactor.output_factors(argument[2], units);

  auto opt = get_options("out_coeffs");
  if (!opt.empty())
    tckfactor.output_coefficients(opt[0][0]);

  if (!debug_path.empty())
    tckfactor.output_all_debug_images(debug_path, "after");

  opt = get_options("out_mu");
  if (!opt.empty()) {
    CONSOLE("Note that while \"-out_mu\" option remains available, "
            "value of proportionality coefficient is also available in output file header comments");
    File::OFStream out_mu(opt[0][0]);
    out_mu << tckfactor.mu();
  }
}
