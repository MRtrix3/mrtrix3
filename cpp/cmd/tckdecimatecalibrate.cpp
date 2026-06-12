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
#include "mrtrix.h"

#include "dwi/tractography/decimate_calibrate.h"

using namespace MR;
using namespace App;
using namespace DWI::Tractography;

// clang-format off
void usage() {

  AUTHOR = "Robert E. Smith (robert.smith@florey.edu.au)";

  SYNOPSIS = "Calibrate the fast streamline decimation density parameter against geometric error";

  DESCRIPTION
  + "The fast decimator exposed by the tckresample -decimate_fast option is governed by a single"
    " dimensionless density knob (mu): the number of output vertices per unit curvature-weighted"
    " arc length. This command sweeps mu over a user-specified range and, for each value, decimates"
    " every streamline in the input tractogram and measures the symmetric Hausdorff distance (in mm)"
    " between the original and decimated tension-Catmull-Rom splines."

  + "For each mu it reports percentiles [50, 75, 95, 99, 99.9, 100] of the distribution of those"
    " per-streamline Hausdorff distances, allowing a value of mu to be selected that bounds the"
    " geometric error introduced by decimation to within a tolerance appropriate for the data. The"
    " mean output/input vertex ratio (compression) is also reported per mu to expose the"
    " fidelity-versus-size trade-off."

  + "The mu range may be specified either as a comma-separated list of explicit values"
    " (e.g. \"1.0,2.0,4.0\") or as a min:step:max range (e.g. \"1.0:1.0:5.0\");"
    " every value must be strictly positive.";

  ARGUMENTS
  + Argument ("in_tracks", "the input track file").type_tracks_in()
  + Argument ("mu", "the set of density values to calibrate over"
                    " (comma-separated list and/or a min:step:max range)").type_sequence_float();

  OPTIONS
  + Option ("csv", "write the calibration table to a CSV file")
    + Argument ("path").type_file_out();

}
// clang-format on

void run() {
  const std::vector<default_type> mu_values = parse_floats(std::string(argument[1]));

  const std::vector<CalibrationRow> rows = decimate_calibrate(argument[0], mu_values);

  std::cout << calibration_table(rows);

  auto opt = get_options("csv");
  if (!opt.empty())
    calibration_save_csv(rows, opt[0][0]);
}
