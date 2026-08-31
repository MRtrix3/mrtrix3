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

#include "command.h"
#include "image.h"

#include "dwi/tractography/properties.h"
#include "dwi/tractography/file.h"
#include "dwi/tractography/roi.h"

#include "dwi/tractography/tracking/exec.h"
#include "dwi/tractography/tracking/method.h"
#include "dwi/tractography/tracking/tractography.h"

#include "dwi/tractography/ACT/act.h"

#include "dwi/tractography/algorithms/fact.h"
#include "dwi/tractography/algorithms/iFOD1.h"
#include "dwi/tractography/algorithms/iFOD2.h"
#include "dwi/tractography/algorithms/nulldist.h"
#include "dwi/tractography/algorithms/sd_stream.h"
#include "dwi/tractography/algorithms/seedtest.h"
#include "dwi/tractography/algorithms/tensor_det.h"
#include "dwi/tractography/algorithms/tensor_prob.h"

#include "dwi/tractography/seeding/seeding.h"

#include "file/ofstream.h"




using namespace MR;
using namespace App;


constexpr float DEFAULT_RETRACK_LENGTH_LIMIT = 0.25f;
constexpr float DEFAULT_TERMINAL_SEARCH_LENGTH = 20.0f;
constexpr float DEFAULT_MIN_WM_LENGTH = 5.0f;
constexpr float DEFAULT_TRUNCATION_MARGIN_LENGTH = 2.0f;
constexpr float DEFAULT_MIN_SGM_LENGTH = 2.0f;

void usage ()
{

  AUTHOR = "Simone Zanoni (simone.zanoni@sydney.edu.au)";

  SYNOPSIS = "Perform streamline truncation based on a 5tt segmentation and re-track streamline terminations";

  DESCRIPTION
    + "This command refines an existing whole-brain tractogram by identifying and correcting biologically implausible " 
    "streamline terminations according to the provided FOD and 5TT images."

    + "The process for each streamline is as follows:"

    + "1. Streamline tissue profiling: a 5TT segmentation is used to determine the underlying dominant tissue type for each vertex of the streamline"

    + "2. Truncation: identification of a suitable point along the streamline where the terminal segment can be truncated due to conflict between streamline trajectory and 5TT"

    + "3. If a suitable truncation point could be identified, that is used as a seed to re-initiate fibre tracking using ACT";

  ARGUMENTS
    + Argument ("tracks_in", "the input file containing the tracks.").type_tracks_in()

    + Argument ("fod_in", "the input file containing the FOD image.").type_image_in()

    + Argument ("5tt_in", "the input file containing the 5tt segmentation.").type_image_in()

    + Argument ("tracks_out", "the output file containing the tracks generated.").type_tracks_out();



  OPTIONS

  + OptionGroup ("Back-tracking/Re-tracking parameters")

  + Option ("retrack_length_limit", "set the maximum allowed length for re-tracking, expressed as a fraction of the original streamline length (default: " + str(DEFAULT_RETRACK_LENGTH_LIMIT) + ")")
    + Argument ("value").type_float(0.0, 1.0)

  + Option ("search_length", "set the length of the terminal region to examine for truncation (in mm) (default: " + str(DEFAULT_TERMINAL_SEARCH_LENGTH) + " mm)")
    + Argument ("value").type_float(0.0)

  + Option ("min_wm_length", "set the minimum length of continuous white matter required to consider a streamline for back-tracking (default: " + str(DEFAULT_MIN_WM_LENGTH) + " mm)")
    + Argument ("value").type_float(0.0)

  + Option ("min_sgm_length", "set the minimum length of continuous sub-cortical grey matter required to consider a termination valid within that tissue (default: " + str(DEFAULT_MIN_SGM_LENGTH) + " mm)")
    + Argument ("value").type_float(0.0)

  + Option ("truncation_margin", "set the distance to truncate back into the white matter from the identified tissue boundary (default: " + str(DEFAULT_TRUNCATION_MARGIN_LENGTH) + " mm)")
    + Argument ("value").type_float(0.0)



  + OptionGroup ("Streamlines tractography options")

  + Option ("step", "set the step size of the algorithm in mm (defaults: for first-order algorithms, " + str(DWI::Tractography::Tracking::Defaults::stepsize_voxels_firstorder, 2) + " x voxelsize; if using RK4, " + str(DWI::Tractography::Tracking::Defaults::stepsize_voxels_rk4, 2) + " x voxelsize; for iFOD2: " + str(DWI::Tractography::Tracking::Defaults::stepsize_voxels_ifod2, 2) + " x voxelsize).")
    + Argument ("size").type_float (0.0)

  + Option ("angle", "set the maximum angle in degrees between successive steps (defaults: " + str(DWI::Tractography::Tracking::Defaults::angle_deterministic) + " for deterministic algorithms; " + str(DWI::Tractography::Tracking::Defaults::angle_ifod1) + " for iFOD1 / nulldist1; " + str(DWI::Tractography::Tracking::Defaults::angle_ifod2) + " for iFOD2 / nulldist2)")
    + Argument ("theta").type_float (0.0)

  + Option ("cutoff", "set the FOD amplitude / fixel size / tensor FA cutoff for terminating tracks (defaults: " + str(DWI::Tractography::Tracking::Defaults::cutoff_fod, 2) + " for FOD-based algorithms; " + str(DWI::Tractography::Tracking::Defaults::cutoff_fixel, 2) + " for fixel-based algorithms; " + str(DWI::Tractography::Tracking::Defaults::cutoff_fa, 2) + " for tensor-based algorithms; threshold multiplied by " + str(DWI::Tractography::Tracking::Defaults::cutoff_act_multiplier) + " when using ACT).")
    + Argument ("value").type_float (0.0)

  + Option ("trials", "set the maximum number of sampling trials at each point (only used for iFOD1 / iFOD2) (default: " + str(DWI::Tractography::Tracking::Defaults::max_trials_per_step) + ").")
    + Argument ("number").type_integer (1)

  + Option ("noprecomputed", "do NOT pre-compute legendre polynomial values. Warning: this will slow down the algorithm by a factor of approximately 4.")

  + Option ("rk4", "use 4th-order Runge-Kutta integration (slower, but eliminates curvature overshoot in 1st-order deterministic methods)");

  REFERENCES
  + "Zanoni, S.; Lv, J.; Smith, R. E. & Calamante, F. "
    "Streamline-Based Analysis: "
    "A novel framework for tractogram-driven streamline-wise statistical analysis. "
    "Proceedings of the International Society for Magnetic Resonance in Medicine, 2025, 4781";

}



void run ()
{
  using namespace DWI::Tractography;
  using namespace DWI::Tractography::Tracking;
  using namespace DWI::Tractography::Algorithms;

  Properties properties;

  size_t count = 0;
  {
    Properties count_properties;
    Streamline<float> tck;
    Reader<float> file (argument[0], count_properties);
    while (file(tck)) {
        ++count;
    }
  }

  Algorithms::load_iFOD2_options (properties);

  Tracking::load_streamline_properties_and_rois (properties);
  properties.compare_stepsize_rois();

  properties["max_num_tracks"] = str(count);

  BacktrackConfig config;
  config.retrack_length_limit = get_option_value ("retrack_length_limit", DEFAULT_RETRACK_LENGTH_LIMIT);
  config.terminal_search_length = get_option_value ("search_length", DEFAULT_TERMINAL_SEARCH_LENGTH);
  config.min_wm_length = get_option_value ("min_wm_length", DEFAULT_MIN_WM_LENGTH);
  config.truncation_margin_length = get_option_value ("truncation_margin", DEFAULT_TRUNCATION_MARGIN_LENGTH);
  config.min_sgm_length = get_option_value ("min_sgm_length", DEFAULT_MIN_SGM_LENGTH);

  if (config.retrack_length_limit <= 0.0f)
    throw Exception ("Retrack length limit (" + str(config.retrack_length_limit) + ") must be positive");

  if (config.min_wm_length <= 2 * config.truncation_margin_length)
    throw Exception ("The minimum WM length (" + str(config.min_wm_length) + " mm) must be greater than twice the truncation margin (" + str(config.truncation_margin_length) + " mm)");

  Exec<iFOD2>::run_backtrack (argument[1], argument[3], argument[0], argument[2], properties, config);
}
