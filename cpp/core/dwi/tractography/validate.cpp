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

#include "dwi/tractography/validate.h"

#include <cmath>
#include <string>

#include "app.h"
#include "dwi/tractography/file.h"
#include "dwi/tractography/properties.h"
#include "dwi/tractography/scalar_file.h"
#include "dwi/tractography/streamline.h"
#include "exception.h"
#include "mrtrix.h"
#include "progressbar.h"

namespace MR::DWI::Tractography {

namespace {

// A thin subclass of Reader<float> that exposes the protected
// get_next_point() method so that raw triplets can be read without
// the high-level streamline-assembly logic filtering them out.
// This allows inspection of delimiter and barrier triplets directly.
class RawTckReader : public Reader<float> {
public:
  RawTckReader(std::string_view file, Properties &props) : Reader<float>(file, props) {}

  // Read one raw 3-float triplet from the binary data section.
  // Returns true if a complete triplet was successfully read, and fills p.
  // Sets at_eof to true when the end of file (or a short read) is detected.
  bool read_triplet(Eigen::Vector3f &p, bool &at_eof) {
    at_eof = false;
    if (!in.is_open() || in.eof()) {
      at_eof = true;
      return false;
    }
    p = get_next_point();
    if (in.fail()) {
      at_eof = in.eof();
      return false;
    }
    return true;
  }
};

} // namespace

TckValidation validate_tck(std::string_view path) {
  Properties props;
  RawTckReader reader(path, props);

  TckValidation result;

  // ---------------------------------------------------------------
  // Extract the header "count" field.
  // ---------------------------------------------------------------
  result.has_header_count = (props.find("count") != props.end());
  if (result.has_header_count)
    result.header_count = to<size_t>(props["count"]);

  // ---------------------------------------------------------------
  // Single-pass scan of every raw triplet in the binary data section.
  //
  // Each triplet must be one of:
  //   - all-finite  → regular vertex in the current streamline
  //   - all-NaN     → inter-streamline delimiter (ends the current streamline)
  //   - all-infinity → end-of-file barrier (must be the last triplet)
  //
  // Any other combination is a format violation and causes an immediate throw.
  // ---------------------------------------------------------------
  size_t current_vertices = 0; // vertices accumulated in the current streamline
  bool has_barrier = false;    // whether the end-of-file barrier has been seen

  ProgressBar progress("Validating tractogram", result.has_header_count ? result.header_count : 0);

  while (true) {
    Eigen::Vector3f p;
    bool at_eof;

    if (!reader.read_triplet(p, at_eof)) {
      // The data section ended before a barrier was seen — the file is truncated.
      if (!has_barrier)
        throw Exception("Tractogram \"" + std::string(path) +
                        "\": binary data section ends without an end-of-file barrier triplet"
                        " (file is truncated or corrupt)");
      // Reached EOF cleanly after the barrier.
      break;
    }

    // Any triplet arriving after the barrier is a format violation.
    if (has_barrier)
      throw Exception("Tractogram \"" + std::string(path) +
                      "\": data present after the end-of-file barrier triplet"
                      " (expected no further data)");

    const bool x_fin = std::isfinite(p[0]);
    const bool y_fin = std::isfinite(p[1]);
    const bool z_fin = std::isfinite(p[2]);
    const bool all_finite = x_fin && y_fin && z_fin;

    const bool x_nan = std::isnan(p[0]);
    const bool y_nan = std::isnan(p[1]);
    const bool z_nan = std::isnan(p[2]);
    const bool all_nan = x_nan && y_nan && z_nan;

    const bool x_inf = std::isinf(p[0]);
    const bool y_inf = std::isinf(p[1]);
    const bool z_inf = std::isinf(p[2]);
    const bool all_inf = x_inf && y_inf && z_inf;

    if (all_finite) {
      // ---------------------------------------------------------------
      // Regular vertex: accumulate into the current streamline.
      // ---------------------------------------------------------------
      ++current_vertices;

    } else if (all_nan) {
      // ---------------------------------------------------------------
      // Delimiter: end of the current streamline.
      // ---------------------------------------------------------------
      if (current_vertices == 0)
        ++result.n_empty;
      else if (current_vertices == 1)
        ++result.n_single_vertex;
      ++result.n_streamlines;
      current_vertices = 0;
      ++progress;

    } else if (all_inf) {
      // ---------------------------------------------------------------
      // End-of-file barrier.
      // Any vertices accumulated without a preceding delimiter indicate a
      // missing terminator for the last streamline.
      // ---------------------------------------------------------------
      if (current_vertices > 0)
        throw Exception("Tractogram \"" + std::string(path) + "\": " + str(current_vertices) +
                        " vertex/vertices in the last streamline have no terminating"
                        " NaN delimiter before the end-of-file barrier");
      has_barrier = true;

    } else {
      // ---------------------------------------------------------------
      // Invalid triplet: partially non-finite, which is neither a valid
      // vertex, a valid delimiter, nor a valid end-of-file marker.
      // ---------------------------------------------------------------
      throw Exception("Tractogram \"" + std::string(path) + "\": invalid triplet (" + str(p[0]) + ", " + str(p[1]) +
                      ", " + str(p[2]) +
                      ") detected — triplets must be all-finite (vertex),"
                      " all-NaN (delimiter), or all-infinity (end-of-file marker)");
    }
  }

  // ---------------------------------------------------------------
  // Post-scan checks on header metadata.
  // ---------------------------------------------------------------
  if (!result.has_header_count)
    throw Exception("Tractogram \"" + std::string(path) + "\": \"count\" field is absent from the file header");

  if (result.n_streamlines != result.header_count)
    throw Exception("Tractogram \"" + std::string(path) + "\": header count (" + str(result.header_count) +
                    ") does not match the number of streamlines read (" + str(result.n_streamlines) + ")");

  return result;
}

void debug_validate_tck(std::string_view path) {
  if (App::log_level < 3)
    return;
  try {
    const TckValidation v = validate_tck(path);
    DEBUG("Tractogram \"" + std::string(path) + "\": " + str(v.n_streamlines) + " streamline(s) verified");
    if (v.n_empty)
      DEBUG("Tractogram \"" + std::string(path) + "\": " + str(v.n_empty) + " empty streamline(s) (0 vertices)");
    if (v.n_single_vertex)
      DEBUG("Tractogram \"" + std::string(path) + "\": " + str(v.n_single_vertex) + " single-vertex streamline(s)");
  } catch (const Exception &e) {
    DEBUG("Tractogram \"" + std::string(path) + "\": validation failed: " + e[0]);
  }
}

void validate_tsf(std::string_view tsf_path, std::string_view tck_path) {
  Properties tsf_props, tck_props;
  ScalarReader<float> tsf_reader(tsf_path, tsf_props);
  Reader<float> tck_reader(tck_path, tck_props);

  // ---------------------------------------------------------------
  // Check 1: timestamps must be present in both headers and must match.
  // The timestamp is copied from the tractogram into the TSF at creation
  // time, so a mismatch indicates the TSF was not produced from this tck.
  // ---------------------------------------------------------------
  {
    const auto tsf_ts = tsf_props.find("timestamp");
    const auto tck_ts = tck_props.find("timestamp");
    if (tsf_ts == tsf_props.end())
      throw Exception("Track scalar file \"" + std::string(tsf_path) +
                      "\": \"timestamp\" field is absent from the header");
    if (tck_ts == tck_props.end())
      throw Exception("Tractogram \"" + std::string(tck_path) + "\": \"timestamp\" field is absent from the header");
    if (tsf_ts->second != tck_ts->second)
      throw Exception("Track scalar file \"" + std::string(tsf_path) + "\" and tractogram \"" + std::string(tck_path) +
                      "\" have different timestamps"
                      " (the scalar file was not produced from this tractogram)");
  }

  // ---------------------------------------------------------------
  // Check 2: "count" field must be present in the TSF header.
  // ---------------------------------------------------------------
  const auto tsf_count_it = tsf_props.find("count");
  if (tsf_count_it == tsf_props.end())
    throw Exception("Track scalar file \"" + std::string(tsf_path) + "\": \"count\" field is absent from the header");
  const size_t tsf_header_count = to<size_t>(tsf_count_it->second);

  // ---------------------------------------------------------------
  // Simultaneous scan of the TSF and the tractogram.
  // For every streamline in the tractogram, read the corresponding
  // scalar sequence from the TSF and compare vertex counts.
  // ---------------------------------------------------------------
  Streamline<float> tck;
  TrackScalar<float> tsf;
  size_t tck_count = 0;
  size_t tsf_count = 0;
  size_t n_length_mismatch = 0;

  {
    const size_t header_count = tck_props.find("count") != tck_props.end() ? to<size_t>(tck_props["count"]) : 0;
    ProgressBar progress("Validating track scalar file", header_count);

    while (tck_reader(tck)) {
      ++tck_count;
      if (tsf_reader(tsf)) {
        ++tsf_count;
        if (tck.size() != tsf.size())
          ++n_length_mismatch;
      }
      ++progress;
    }

    // Drain any remaining scalar sequences beyond the tck streamline count.
    while (tsf_reader(tsf)) {
      ++tsf_count;
      ++progress;
    }
  }

  // ---------------------------------------------------------------
  // Check 3: TSF header count must match actual TSF streamline count.
  // ---------------------------------------------------------------
  if (tsf_count != tsf_header_count)
    throw Exception("Track scalar file \"" + std::string(tsf_path) + "\": header \"count\" (" + str(tsf_header_count) +
                    ") does not match the number of scalar sequences read (" + str(tsf_count) + ")");

  // ---------------------------------------------------------------
  // Check 4: TSF streamline count must match tck streamline count.
  // ---------------------------------------------------------------
  if (tsf_count != tck_count)
    throw Exception("Track scalar file \"" + std::string(tsf_path) + "\" contains " + str(tsf_count) +
                    " scalar sequence(s),"
                    " but tractogram \"" +
                    std::string(tck_path) + "\" contains " + str(tck_count) + " streamline(s)");

  // ---------------------------------------------------------------
  // Check 5: per-streamline vertex counts must match.
  // ---------------------------------------------------------------
  if (n_length_mismatch)
    throw Exception("Track scalar file \"" + std::string(tsf_path) + "\": " + str(n_length_mismatch) + " of " +
                    str(tck_count) +
                    " streamline(s) have a different number of vertices"
                    " between the scalar file and the tractogram");
}

void debug_validate_tsf(std::string_view tsf_path, std::string_view tck_path) {
  if (App::log_level < 3)
    return;
  try {
    validate_tsf(tsf_path, tck_path);
    DEBUG("Track scalar file \"" + std::string(tsf_path) +
          "\""
          " validated against tractogram \"" +
          std::string(tck_path) + "\": OK");
  } catch (const Exception &e) {
    DEBUG("Track scalar file \"" + std::string(tsf_path) +
          "\""
          " vs tractogram \"" +
          std::string(tck_path) +
          "\":"
          " validation failed: " +
          e[0]);
  }
}

} // namespace MR::DWI::Tractography
