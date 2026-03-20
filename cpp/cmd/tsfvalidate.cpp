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
#include "dwi/tractography/file.h"
#include "dwi/tractography/properties.h"
#include "dwi/tractography/scalar_file.h"
#include "dwi/tractography/streamline.h"
#include "dwi/tractography/trx_utils.h"
#include "progressbar.h"
#include "types.h"

using namespace MR;
using namespace MR::DWI::Tractography;
using namespace App;

// clang-format off
void usage() {

  AUTHOR = "Robert E. Smith (robert.smith@florey.edu.au)";

  SYNOPSIS = "Validate a track scalar file against the corresponding track data";

  DESCRIPTION
  + "For TSF/TCK pairs, performs the standard header and data validation. "
    "For TRX input, validates that the named dpv field has the correct number of "
    "vertices (matching the TRX streamline offsets).";

  ARGUMENTS
  + Argument ("tsf", "the input track scalar file, or the TRX tractogram when using -field").type_file_in()
  + Argument ("tracks", "the track file on which the TSF is based (.tck or .trx)").type_file_in();

  OPTIONS
  + Option ("field", "name of the dpv field to validate (required when the tracks argument is a TRX file)")
    + Argument ("name").type_text();

}
// clang-format on

typedef float value_type;

void run() {
  const std::string tsf_path(argument[0]);
  const std::string tck_path(argument[1]);

  auto field_opt = get_options("field");

  // TRX mode: validate that the named dpv field vertex count matches num_vertices()
  if (TRX::is_trx(tck_path)) {
    if (field_opt.empty())
      throw Exception("Use -field to specify the dpv field to validate when the tracks argument is a TRX file");

    const std::string field_name(field_opt[0][0]);
    auto trx = TRX::load_trx_header_only(tck_path);
    if (!trx)
      throw Exception("Failed to load TRX file: " + tck_path);

    const size_t n_vertices = trx->num_vertices();
    const size_t n_streamlines = trx->num_streamlines();

    auto it = trx->data_per_vertex.find(field_name);
    if (it == trx->data_per_vertex.end() || !it->second)
      throw Exception("TRX file has no dpv field '" + field_name + "'");

    const size_t dpv_rows = static_cast<size_t>(it->second->_data.rows());

    size_t error_count = 0;
    if (dpv_rows != n_vertices) {
      CONSOLE("dpv field '" + field_name + "' has " + str(dpv_rows) + " rows but TRX has " + str(n_vertices) +
              " vertices");
      ++error_count;
    }

    // Also check that per-streamline vertex counts sum to total vertices
    // (validates that offsets are self-consistent).
    size_t offset_sum = 0;
    for (size_t i = 0; i < n_streamlines; ++i) {
      const size_t start = static_cast<size_t>(trx->streamlines->_offsets(static_cast<Eigen::Index>(i), 0));
      const size_t end = static_cast<size_t>(trx->streamlines->_offsets(static_cast<Eigen::Index>(i + 1), 0));
      offset_sum += end - start;
    }
    if (offset_sum != n_vertices) {
      CONSOLE("TRX streamline offsets sum to " + str(offset_sum) + " vertices but num_vertices() reports " +
              str(n_vertices));
      ++error_count;
    }

    if (error_count)
      throw Exception("Error" + std::string(error_count > 1 ? "s" : "") + " detected");
    else
      CONSOLE("TRX dpv field '" + field_name + "' checked OK (" + str(dpv_rows) + " vertices, " + str(n_streamlines) +
              " streamlines)");
    return;
  }

  // TSF/TCK mode: existing validation logic
  if (!field_opt.empty())
    WARN("-field is ignored when the tracks argument is a TCK file");

  Properties tsf_properties, tck_properties;
  ScalarReader<value_type> tsf_reader(tsf_path, tsf_properties);
  Reader<value_type> tck_reader(tck_path, tck_properties);
  size_t error_count = 0;

  Properties::const_iterator tsf_count_field = tsf_properties.find("count");
  Properties::const_iterator tck_count_field = tck_properties.find("count");
  size_t tsf_header_count = 0, tck_header_count = 0;
  if (tsf_count_field == tsf_properties.end() || tck_count_field == tck_properties.end()) {
    WARN("Unable to verify equal track counts: \"count\" field absent from file header");
  } else {
    tsf_header_count = to<size_t>(tsf_count_field->second);
    tck_header_count = to<size_t>(tck_count_field->second);
    if (tsf_header_count != tck_header_count) {
      CONSOLE("\"count\" fields in file headers do not match");
      ++error_count;
    }
  }

  Properties::const_iterator tsf_timestamp_field = tsf_properties.find("timestamp");
  Properties::const_iterator tck_timestamp_field = tck_properties.find("timestamp");
  if (tsf_timestamp_field == tsf_properties.end() || tck_timestamp_field == tck_properties.end()) {
    WARN("Unable to verify equal file timestamps: \"timestamp\" field absent from file header");
  } else if (tsf_timestamp_field->second != tck_timestamp_field->second) {
    CONSOLE("\"timestamp\" fields in file headers do not match");
    ++error_count;
  }

  Streamline<value_type> track;
  TrackScalar<value_type> scalar;
  size_t tck_counter = 0, tsf_counter = 0, length_mismatch_count = 0;

  {
    ProgressBar progress("Validating track scalar file", tck_header_count);
    while (tck_reader(track)) {
      ++tck_counter;
      if (tsf_reader(scalar)) {
        ++tsf_counter;
        if (track.size() != scalar.size())
          ++length_mismatch_count;
      }
      ++progress;
    }

    while (tsf_reader(scalar)) {
      ++tsf_counter;
      ++progress;
    }
  }

  if (tsf_header_count && tsf_counter != tsf_header_count) {
    CONSOLE("Actual number of tracks counted in scalar file (" + str(tsf_counter) +
            ") does not match number reported in header (" + str(tsf_header_count) + ")");
    ++error_count;
  }
  if (tck_header_count && tck_counter != tck_header_count) {
    CONSOLE("Actual number of tracks counted in track file (" + str(tck_counter) +
            ") does not match number reported in header (" + str(tck_header_count) + ")");
    ++error_count;
  }
  if (tck_counter != tsf_counter) {
    CONSOLE("Actual number of tracks counter in scalar file (" + str(tsf_counter) +
            ") does not match actual number of tracks counted in track file (" + str(tck_counter) + ")");
    ++error_count;
  }
  if (length_mismatch_count) {
    CONSOLE(str(length_mismatch_count) + " track" + (length_mismatch_count == 1 ? " was" : "s were") +
            " detected with different lengths between track and scalar data");
    ++error_count;
  }
  if (error_count > 1) {
    throw Exception("Multiple errors detected");
  } else if (error_count) {
    throw Exception("Error detected");
  } else {
    CONSOLE("Track scalar file data checked OK");
  }
}
