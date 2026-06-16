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
#include "dwi/tractography/formats/tsf.h"
#include "dwi/tractography/properties.h"
#include "dwi/tractography/streamline.h"
#include "dwi/tractography/tractogram.h"
#include "dwi/tractography/tractogram_item.h"
#include "exception.h"
#include "file/matrix.h"
#include "mrtrix.h"
#include "progressbar.h"

namespace MR::DWI::Tractography {

namespace {

// A thin subclass of TCKReader<float> that exposes the protected
// get_next_point() method so that raw triplets can be read without
// the high-level streamline-assembly logic filtering them out.
// This allows inspection of delimiter and barrier triplets directly.
class RawTckReader : public TCKReader<float> {
public:
  RawTckReader(const std::filesystem::path &file, Properties &props) : TCKReader<float>(file, props) {}

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

TckValidation validate_tck(const std::filesystem::path &path) {
  Properties props;
  RawTckReader reader(path, props);

  TckValidation result;

  // ---------------------------------------------------------------
  // Extract the header "count" field.
  // ---------------------------------------------------------------
  if (props.find("count") != props.end())
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
  bool seen_eof_barrier = false;

  ProgressBar progress("Validating tractogram", result.header_count.has_value() ? *result.header_count : 0);

  while (true) {
    Eigen::Vector3f p;
    bool at_eof;

    if (!reader.read_triplet(p, at_eof)) {
      // The data section ended before a barrier was seen — the file is truncated.
      if (!seen_eof_barrier)
        throw Exception("Tractogram \"" + path.string() + "\": " +                            //
                        " binary data section ends without an end-of-file barrier triplet;" + //
                        " file may be truncated or corrupt");                                 //
      // Reached EOF cleanly after the barrier.
      break;
    }

    // Any triplet arriving after the barrier is a format violation.
    if (seen_eof_barrier)
      throw Exception("Tractogram \"" + path.string() + "\":" +                      //
                      " vertex data present after the end-of-file barrier triplet"); //

    const bool all_nan = std::isnan(p[0]) && std::isnan(p[1]) && std::isnan(p[2]);
    const bool all_inf = std::isinf(p[0]) && std::isinf(p[1]) && std::isinf(p[2]);

    if (p.allFinite()) {
      // ---------------------------------------------------------------
      // Regular vertex: accumulate into the current streamline.
      // ---------------------------------------------------------------
      ++current_vertices;

    } else if (all_nan) {
      // ---------------------------------------------------------------
      // Delimiter: end of the current streamline.
      // ---------------------------------------------------------------
      switch (current_vertices) {
      case 0:
        ++result.n_empty;
        break;
      case 1:
        ++result.n_single_vertex;
        break;
      default:
        break;
      }
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
        throw Exception("Tractogram \"" + path.string() + "\": " +                                 //
                        str(current_vertices) + (current_vertices > 1 ? " vertices" : " vertex") + //
                        " in the last streamline," +                                               //
                        " which has no terminating NaN delimiter before the end-of-file barrier"); //
      seen_eof_barrier = true;

    } else {
      // ---------------------------------------------------------------
      // Invalid triplet: partially non-finite, which is neither a valid
      // vertex, a valid delimiter, nor a valid end-of-file marker.
      // ---------------------------------------------------------------
      throw Exception("Tractogram \"" + path.string() + "\":" +                       //
                      " invalid partially-finite triplet detected;" +                 //
                      " triplets must be all-finite (vertex), all-NaN (delimiter)," + //
                      " or all-infinity (end-of-file marker)");                       //
    }
  }

  // ---------------------------------------------------------------
  // Post-scan checks on header metadata.
  // ---------------------------------------------------------------
  if (result.header_count.has_value() && result.n_streamlines != *result.header_count)
    throw Exception("Tractogram \"" + path.string() + "\":" +                                              //
                    " header count (" + str(*result.header_count) + ")" +                                  //
                    " does not match the number of streamlines read (" + str(result.n_streamlines) + ")"); //

  return result;
}

TractogramValidation validate_tractogram(const std::filesystem::path &path) {
  Properties properties;
  // Read through the format-handler framework so any supported format is
  //   accepted, not just ".tck" (step 8).
  auto tractogram = Tractogram<float>::open(path, properties);

  TractogramValidation result;
  if (properties.find("count") != properties.end())
    result.header_count = to<size_t>(properties["count"]);

  ProgressBar progress("Validating tractogram", result.header_count.has_value() ? *result.header_count : 0);
  TractogramItem<float> item;
  while (tractogram.read(item)) {
    result.vertices_per_streamline.push_back(item.streamline.size());
    ++result.n_streamlines;
    ++progress;
  }

  if (result.header_count.has_value() && result.n_streamlines != *result.header_count)
    throw Exception("Tractogram \"" + path.string() + "\":" +                                              //
                    " header count (" + str(*result.header_count) + ")" +                                  //
                    " does not match the number of streamlines read (" + str(result.n_streamlines) + ")"); //

  return result;
}

void validate_dps_field(const std::filesystem::path &dps_path,
                        std::string_view field_name,
                        const TractogramValidation &validation) {
  // A per-streamline (dps) field is a single value or one fixed-length row per
  //   streamline: exactly one row per streamline of the tractogram (step 8).
  const auto data = File::Matrix::load_matrix<double>(dps_path);
  const size_t n_entries = static_cast<size_t>(data.rows());
  if (n_entries != validation.n_streamlines)
    throw Exception("Data-per-streamline field \"" + std::string(field_name) + "\"" +                    //
                    " (\"" + dps_path.string() + "\")" +                                                 //
                    " has " + str(n_entries) + " entries," +                                             //
                    " but the tractogram contains " + str(validation.n_streamlines) + " streamline(s)"); //
}

void validate_dpv_field(const std::filesystem::path &tsf_path,
                        std::string_view field_name,
                        const TractogramValidation &validation) {
  // A per-vertex (dpv) field has one scalar sequence per streamline, each of
  //   length equal to that streamline's vertex count (step 8).
  Properties tsf_properties;
  ScalarReader<float> reader(tsf_path, tsf_properties);
  TrackScalar<float> scalar;
  size_t index = 0;
  size_t n_length_mismatch = 0;
  while (reader(scalar)) {
    if (index < validation.vertices_per_streamline.size() && scalar.size() != validation.vertices_per_streamline[index])
      ++n_length_mismatch;
    ++index;
  }
  if (index != validation.n_streamlines)
    throw Exception("Data-per-vertex field \"" + std::string(field_name) + "\"" +     //
                    " (\"" + tsf_path.string() + "\")" +                              //
                    " contains " + str(index) + " scalar sequence(s)," +              //
                    " but the tractogram contains " + str(validation.n_streamlines) + //
                    " streamline(s)");                                                //
  if (n_length_mismatch > 0)
    throw Exception("Data-per-vertex field \"" + std::string(field_name) + "\"" +                        //
                    " (\"" + tsf_path.string() + "\"): " +                                               //
                    str(n_length_mismatch) + " of " + str(validation.n_streamlines) + " streamline(s)" + //
                    " have a per-vertex sequence length differing from the streamline vertex count");    //
}

void validate_tsf_properties(const Properties &a, const Properties &b, std::string_view file_types) {
  const Properties::const_iterator stamp_a = a.find("timestamp");
  const Properties::const_iterator stamp_b = b.find("timestamp");
  if (stamp_a == a.end() || stamp_b == b.end())
    throw Exception("Unable to verify " + file_types + ": missing timestamp");
  if (stamp_a->second != stamp_b->second) {
    throw Exception("Invalid " + file_types + ":" +                     //
                    " timestamps do not match," +                       //
                    " suggesting differing originating tractogram(s)"); //
  }

  const Properties::const_iterator count_a = a.find("count");
  const Properties::const_iterator count_b = b.find("count");
  if ((count_a == a.end()) || (count_b == b.end()))
    throw Exception("Unable to validate " + file_types + ": missing count field");
  if (to<size_t>(count_a->second) != to<size_t>(count_b->second))
    throw Exception(file_types + " does not contain same number of streamlines");
}

void validate_tsf(const std::filesystem::path &tsf_path, const std::filesystem::path &tck_path) {
  Properties tsf_props;
  Properties tck_props;
  ScalarReader<float> tsf_reader(tsf_path, tsf_props);
  TCKReader<float> tck_reader(tck_path, tck_props);

  // ---------------------------------------------------------------
  // Check 1: timestamps must be present in both headers and must match.
  // The timestamp is copied from the tractogram into the TSF at creation
  // time, so a mismatch indicates the TSF was not produced from this tck.

  // Check 2: "count" field must be present in the TSF header.
  // ---------------------------------------------------------------
  validate_tsf_properties(tsf_props, tck_props, ".tck / .tsf pair");

  const size_t header_count = to<size_t>(tsf_props["count"]);

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
  if (tsf_count != header_count)
    throw Exception("Track scalar file \"" + tsf_path.string() + "\":" +                             //
                    " header \"count\" (" + str(header_count) + ")" +                                //
                    " does not match the number of scalar sequences read (" + str(tsf_count) + ")"); //

  // ---------------------------------------------------------------
  // Check 4: TSF streamline count must match tck streamline count.
  // ---------------------------------------------------------------
  if (tsf_count != tck_count)
    throw Exception("Track scalar file \"" + tsf_path.string() + "\"" +      //
                    " contains " + str(tsf_count) + " scalar sequence(s)," + //
                    " but tractogram \"" + tck_path.string() + "\"" +        //
                    " contains " + str(tck_count) + " streamline(s)");       //

  // ---------------------------------------------------------------
  // Check 5: per-streamline vertex counts must match.
  // ---------------------------------------------------------------
  if (n_length_mismatch > 0)
    throw Exception("Track scalar file \"" + tsf_path.string() + "\": " +                               //
                    str(n_length_mismatch) + " of " + str(tck_count) + " streamlines" +                 //
                    " have a different number of vertices between the scalar file and the tractogram"); //
}

} // namespace MR::DWI::Tractography
