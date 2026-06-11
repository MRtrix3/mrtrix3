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

#include "command.h"
#include "mrtrix.h"

#include "dwi/tractography/validate.h"

using namespace MR;
using namespace App;
using namespace MR::DWI::Tractography;

// clang-format off
void usage() {

  AUTHOR = "Robert E. Smith (robert.smith@florey.edu.au)";

  SYNOPSIS = "Validate a tractogram file and its associated sidecar data";

  DESCRIPTION
  + "This command checks that a tractogram file is well-formed."
    " For the in-house \".tck\" format,"
    " the binary data section consists of a sequence of 3-float triplets,"
    " each of which must be exactly one of:"
    " a regular vertex (all three components finite),"
    " an inter-streamline delimiter (all three components NaN),"
    " or the mandatory end-of-file barrier (all three components infinity);"
    " the end-of-file barrier must be the last triplet in the file."

  + "The following hard violations cause the command to fail for a \".tck\" file:"
    " (1) a triplet that is partially non-finite"
    " (i.e. not all-finite, not all-NaN, and not all-infinity);"
    " (2) any data present after the end-of-file barrier;"
    " (3) the end of the binary data section is reached without an end-of-file barrier"
    " (truncated file);"
    " (4) the last streamline body is not terminated by a NaN delimiter"
    " before the end-of-file barrier;"
    " (5) the \"count\" field is absent from the file header;"
    " (6) the \"count\" field in the file header does not match"
    " the number of streamlines actually present in the file."

  + "For any supported tractography format,"
    " the entire tractogram is read"
    " and the \"count\" field in the header (where present)"
    " is checked against the number of streamlines actually read."

  + "Associated sidecar data can additionally be validated against the tractogram"
    " using the -tsf and -tck_weights options;"
    " every per-streamline (data-per-streamline) field must contain exactly one entry per streamline,"
    " and every per-vertex (data-per-vertex) field must contain"
    " one scalar sequence per streamline whose length matches that streamline's vertex count."

  + "The command also reports the presence of streamlines"
    " with zero vertices or exactly one vertex,"
    " which are degenerate cases that may indicate issues with the"
    " tractography algorithm that produced the file.";

  ARGUMENTS
  + Argument ("tracks_in", "the input tractogram file").type_tracks_in();

  OPTIONS
  + Option ("tsf", "validate a per-vertex (data-per-vertex) track scalar file (.tsf)"
                   " against the tractogram"
                   " (may be specified multiple times)").allow_multiple()
    + Argument ("path").type_file_in()
  + Option ("tck_weights", "validate a per-streamline (data-per-streamline) numerical sidecar file"
                           " (plain-text / .csv / .npy) against the tractogram"
                           " (may be specified multiple times)").allow_multiple()
    + Argument ("path").type_file_in();
}
// clang-format on

void run() {
  const std::filesystem::path tracks_path{argument[0]};
  const bool is_tck = tracks_path.extension() == ".tck";

  // For the in-house ".tck" format, retain the exhaustive triplet-level scan
  //   (delimiter / barrier / partial-finite checks) of validate_tck(); the
  //   format-agnostic validate_tractogram() reads any supported format and
  //   checks the header/content count and records per-streamline vertex counts.
  TractogramValidation validation;
  if (is_tck) {
    const TckValidation tck = validate_tck(tracks_path);
    validation.header_count = tck.header_count;
    validation.n_streamlines = tck.n_streamlines;
    if (tck.n_empty > 0) {
      WARN(str(tck.n_empty) + " empty streamline(s) (0 vertices) found");
    }
    if (tck.n_single_vertex > 0) {
      WARN(str(tck.n_single_vertex) + " single-vertex streamline(s) found");
    }
    if (tck.n_empty == 0 && tck.n_single_vertex == 0) {
      CONSOLE("All streamlines have two or more vertices");
    }
  }

  // Read the whole tractogram through the framework: this validates the
  //   header/content count for every supported format, and (crucially) provides
  //   the per-streamline vertex counts needed to validate any dpv sidecar.
  const bool need_vertices = !get_options("tsf").empty();
  if (!is_tck || need_vertices)
    validation = validate_tractogram(tracks_path);

  CONSOLE("Tractogram \"" + argument[0].as_text() + "\" is valid: " + //
          str(validation.n_streamlines) + " streamline(s)");          //

  // Validate each requested sidecar field, generalising validate_tsf across all
  //   dps/dpv fields supplied for the tractogram (step 8).
  for (const auto &opt : get_options("tck_weights")) {
    const std::filesystem::path dps_path{opt[0]};
    validate_dps_field(dps_path, dps_path.stem().string(), validation);
    CONSOLE("Data-per-streamline field \"" + dps_path.string() + "\" is consistent with the tractogram");
  }
  for (const auto &opt : get_options("tsf")) {
    const std::filesystem::path dpv_path{opt[0]};
    validate_dpv_field(dpv_path, dpv_path.stem().string(), validation);
    CONSOLE("Data-per-vertex field \"" + dpv_path.string() + "\" is consistent with the tractogram");
  }
}
