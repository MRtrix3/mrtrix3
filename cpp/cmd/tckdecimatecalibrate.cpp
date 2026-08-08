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
#include "enum.h"
#include "mrtrix.h"

#include "dwi/tractography/decimate_calibrate.h"

using namespace MR;
using namespace App;
using namespace DWI::Tractography;

// clang-format off
void usage() {

  AUTHOR = "Robert E. Smith (robert.smith@florey.edu.au)";

  SYNOPSIS = "Calibrate a streamline decimation resampler against geometric error and computational cost";

  DESCRIPTION
  + "This command evaluates one of the two streamline decimation resamplers exposed by tckresample:"
    " the curvature-adaptive single-pass \"fast\" decimator (the -decimate_fast option), governed by"
    " a dimensionless density knob mu (output vertices per unit curvature-weighted arc length;"
    " larger values retain more vertices); and the greedy knot-insertion \"slow\" decimator (the"
    " -decimate_slow option), governed directly by a deviation tolerance in mm (smaller values retain"
    " more vertices). Use the -algorithm option to select which resampler to calibrate (default: fast)."

  + "For every streamline in the input tractogram and every value in the swept parameter set, the"
    " selected decimator is run and the symmetric Hausdorff distance (in mm) between the original and"
    " decimated tension-Catmull-Rom splines is measured. Per parameter value the command reports"
    " percentiles [50, 75, 95, 99, 99.9, 100] of that per-streamline distance distribution, the mean"
    " output/input vertex ratio (compression), the mean absolute output vertex count, and the total"
    " time spent inside the decimator (summed across all streamlines; the resampling cost only,"
    " excluding the Hausdorff measurement)."

  + "For the fast algorithm the swept values are mu, and the Hausdorff percentiles expose the"
    " geometric error that the chosen density incurs. For the slow algorithm the swept values are the"
    " deviation tolerances themselves; the reconstruction is bounded within the tolerance by"
    " construction, so the headline output is the compression ratio achieved at each tolerance, while"
    " the reported Hausdorff percentiles verify that the spline-level error respects the bound."

  + "The parameter set may be specified either as a comma-separated list of explicit values"
    " (e.g. \"1.0,2.0,4.0\") or as a min:step:max range (e.g. \"1.0:1.0:5.0\");"
    " every value must be strictly positive.";

  ARGUMENTS
  + Argument ("in_tracks", "the input track file").type_tracks_in()
  + Argument ("values", "the set of decimation parameter values to calibrate over"
                        " (comma-separated list and/or a min:step:max range);"
                        " interpreted as the fast decimator density knob mu by default,"
                        " or as the slow decimator Hausdorff-distance tolerance (mm) under -algorithm slow")
              .type_sequence_float();

  OPTIONS
  + Option ("algorithm", "the decimation resampler to calibrate; one of "
                         + MR::Enum::join<DecimateAlgorithm>() + " (default: fast)")
    + Argument ("name").type_choice<DecimateAlgorithm>()

  + Option ("csv", "write the calibration table to a CSV file")
    + Argument ("path").type_file_out();

}
// clang-format on

void run() {
  const std::vector<default_type> values = parse_floats(std::string(argument[1]));

  const DecimateAlgorithm algorithm = get_option_choice("algorithm", DecimateAlgorithm::Fast);

  const std::vector<CalibrationRow> rows = decimate_calibrate(argument[0], values, algorithm);

  std::cout << calibration_table(rows, algorithm);

  auto opt = get_options("csv");
  if (!opt.empty())
    calibration_save_csv(rows, algorithm, opt[0][0]);
}
