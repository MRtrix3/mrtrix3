/* Copyright (c) 2008-2017 the MRtrix3 contributors.
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


#include "command.h"
#include "exception.h"
#include "image.h"
#include "header.h"

#include "file/path.h"

#include "dwi/directions/set.h"

#include "dwi/tractography/mapping/fixel_td_map.h"

#include "dwi/tractography/SIFT/proc_mask.h"
#include "dwi/tractography/SIFT/sift.h"

#include "dwi/tractography/SIFT2/tckfactor.h"





using namespace MR;
using namespace App;

using namespace MR::DWI;
using namespace MR::DWI::Tractography;
using namespace MR::DWI::Tractography::SIFT2;




const OptionGroup SIFT2RegularisationOption = OptionGroup ("Regularisation options for SIFT2")

  + Option ("reg_tikhonov", "provide coefficient for regularising streamline weighting coefficients (Tikhonov regularisation) (default: " + str(SIFT2_REGULARISATION_TIKHONOV_DEFAULT, 2) + ")")
    + Argument ("value").type_float (0.0)

  + Option ("reg_tv", "provide coefficient for regularising variance of streamline weighting coefficient to fixels along its length (Total Variation regularisation) (default: " + str(SIFT2_REGULARISATION_TV_DEFAULT, 2) + ")")
    + Argument ("value").type_float (0.0);



const OptionGroup SIFT2AlgorithmOption = OptionGroup ("Options for controlling the SIFT2 optimisation algorithm")

  + Option ("min_td_frac", "minimum fraction of the FOD integral reconstructed by streamlines; "
                           "if the reconstructed streamline density is below this fraction, the fixel is excluded from optimisation "
                           "(default: " + str(SIFT2_MIN_TD_FRAC_DEFAULT, 2) + ")")
    + Argument ("fraction").type_float (0.0, 1.0)

  + Option ("min_iters", "minimum number of iterations to run before testing for convergence; "
                         "this can prevent premature termination at early iterations if the cost function increases slightly "
                         "(default: " + str(SIFT2_MIN_ITERS_DEFAULT) + ")")
    + Argument ("count").type_integer (0)

  + Option ("max_iters", "maximum number of iterations to run before terminating program")
    + Argument ("count").type_integer (0)

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
    + Argument ("step").type_float ()

  + Option ("min_cf_decrease", "minimum decrease in the cost function (as a fraction of the initial value) that must occur each iteration for the algorithm to continue "
                               "(default: " + str(SIFT2_MIN_CF_DECREASE_DEFAULT, 2) + ")")
    + Argument ("frac").type_float (0.0, 1.0);





void usage ()
{

  AUTHOR = "Robert E. Smith (robert.smith@florey.edu.au)";

  SYNOPSIS = "Successor to the SIFT method; instead of removing streamlines, use an EM framework to find an appropriate cross-section multiplier for each streamline";

  REFERENCES
    + "Smith, R. E.; Tournier, J.-D.; Calamante, F. & Connelly, A. " // Internal
    "SIFT2: Enabling dense quantitative assessment of brain white matter connectivity using streamlines tractography. "
    "NeuroImage, 2015, 119, 338-351";

  ARGUMENTS
  + Argument ("in_tracks",   "the input track file").type_tracks_in()
  + Argument ("in_fod",      "input image containing the spherical harmonics of the fibre orientation distributions").type_image_in()
  + Argument ("out_weights", "output text file containing the weighting factor for each streamline").type_file_out();

  OPTIONS

  + SIFT::SIFTModelProcMaskOption
  + SIFT::SIFTModelOption
  + SIFT::SIFTOutputOption

  + Option ("out_coeffs", "output text file containing the weighting coefficient for each streamline")
    + Argument ("path").type_file_out()

  + SIFT2RegularisationOption
  + SIFT2AlgorithmOption;

}




void run ()
{

  if (get_options("min_factor").size() && get_options("min_coeff").size())
    throw Exception ("Options -min_factor and -min_coeff are mutually exclusive");
  if (get_options("max_factor").size() && get_options("max_coeff").size())
    throw Exception ("Options -max_factor and -max_coeff are mutually exclusive");

  if (Path::has_suffix (argument[2], ".tck"))
    throw Exception ("Output of tcksift2 command should be a text file, not a tracks file");

  auto in_dwi = Image<float>::open (argument[1]);

  DWI::Directions::FastLookupSet dirs (1281);

  TckFactor tckfactor (in_dwi, dirs);

  const bool output_debug = get_options ("output_debug").size();

  if (output_debug)
    tckfactor.output_proc_mask ("proc_mask.mif");

  tckfactor.perform_FOD_segmentation (in_dwi);
  tckfactor.scale_FDs_by_GM();

  tckfactor.map_streamlines (argument[0]);

  tckfactor.store_orig_TDs();

  const float min_td_frac = get_option_value ("min_td_frac", SIFT2_MIN_TD_FRAC_DEFAULT);
  tckfactor.remove_excluded_fixels (min_td_frac);

  if (output_debug)
    tckfactor.output_all_debug_images ("before");

  auto opt = get_options ("csv");
  if (opt.size())
    tckfactor.set_csv_path (opt[0][0]);

  const float reg_tikhonov = get_option_value ("reg_tikhonov", SIFT2_REGULARISATION_TIKHONOV_DEFAULT);
  const float reg_tv = get_option_value ("reg_tv", SIFT2_REGULARISATION_TV_DEFAULT);
  tckfactor.set_reg_lambdas (reg_tikhonov, reg_tv);

  opt = get_options ("min_iters");
  if (opt.size())
    tckfactor.set_min_iters (int(opt[0][0]));
  opt = get_options ("max_iters");
  if (opt.size())
    tckfactor.set_max_iters (int(opt[0][0]));
  opt = get_options ("min_factor");
  if (opt.size())
    tckfactor.set_min_factor (float(opt[0][0]));
  opt = get_options ("min_coeff");
  if (opt.size())
    tckfactor.set_min_coeff (float(opt[0][0]));
  opt = get_options ("max_factor");
  if (opt.size())
    tckfactor.set_max_factor (float(opt[0][0]));
  opt = get_options ("max_coeff");
  if (opt.size())
    tckfactor.set_max_coeff (float(opt[0][0]));
  opt = get_options ("max_coeff_step");
  if (opt.size())
    tckfactor.set_max_coeff_step (float(opt[0][0]));
  opt = get_options ("min_cf_decrease");
  if (opt.size())
    tckfactor.set_min_cf_decrease (float(opt[0][0]));

  tckfactor.estimate_factors();

  tckfactor.output_factors (argument[2]);

  opt = get_options ("out_coeffs");
  if (opt.size())
    tckfactor.output_coefficients (opt[0][0]);

  if (output_debug)
    tckfactor.output_all_debug_images ("after");

  opt = get_options ("out_mu");
  if (opt.size()) {
    File::OFStream out_mu (opt[0][0]);
    out_mu << tckfactor.mu();
  }

}


