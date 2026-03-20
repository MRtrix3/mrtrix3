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
#include "dwi/tractography/properties.h"
#include "dwi/tractography/scalar_file.h"
#include "dwi/tractography/streamline.h"
#include "dwi/tractography/trx_utils.h"

using namespace MR;
using namespace App;

// clang-format off
void usage() {

  AUTHOR = "David Raffelt (david.raffelt@florey.edu.au)";

  SYNOPSIS = "Threshold and invert track scalar files";

  DESCRIPTION
  + "Accepts .tsf track scalar files or TRX dpv fields as input and output. "
    "Use -field_in / -field_out to name the dpv fields when the corresponding "
    "argument is a TRX file.";

  ARGUMENTS
  + Argument ("input",  "the input track scalar file or TRX tractogram").type_file_in()
  + Argument ("T",      "the desired threshold").type_float()
  + Argument ("output", "the binary output track scalar file or TRX tractogram").type_file_out();

  OPTIONS
  + Option ("invert", "invert the output mask")

  + Option ("field_in", "dpv field name to read from input TRX file")
    + Argument ("name").type_text()

  + Option ("field_out", "dpv field name to write to output TRX file "
                         "(output TRX must already exist; use the same path as input to add in-place)")
    + Argument ("name").type_text();

}
// clang-format on

using value_type = float;

void run() {
  const std::string input_path(argument[0]);
  const float threshold = argument[1];
  const std::string output_path(argument[2]);
  const bool invert = !get_options("invert").empty();

  const bool trx_in = DWI::Tractography::TRX::is_trx(input_path);
  const bool trx_out = DWI::Tractography::TRX::is_trx(output_path);

  auto field_in_opt = get_options("field_in");
  auto field_out_opt = get_options("field_out");

  if (trx_in && field_in_opt.empty())
    throw Exception("Use -field_in to specify the dpv field when the input is a TRX file");
  if (trx_out && field_out_opt.empty())
    throw Exception("Use -field_out to specify the dpv field when the output is a TRX file");
  if (!trx_in && !field_in_opt.empty())
    WARN("-field_in is ignored for non-TRX input");
  if (!trx_out && !field_out_opt.empty())
    WARN("-field_out is ignored for non-TRX output");

  const std::string field_in = trx_in ? std::string(field_in_opt[0][0]) : "";
  const std::string field_out = trx_out ? std::string(field_out_opt[0][0]) : "";

  DWI::Tractography::TrackScalar<value_type> tck_scalar, tck_mask;

  if (trx_in) {
    // Read all scalars, process them, then write output
    std::vector<DWI::Tractography::TrackScalar<value_type>> results;
    {
      DWI::Tractography::TRX::TRXScalarReader reader(input_path, field_in);
      results.reserve(reader.num_streamlines());
      while (reader(tck_scalar)) {
        tck_mask.set_index(tck_scalar.get_index());
        tck_mask.resize(tck_scalar.size());
        for (size_t i = 0; i < tck_scalar.size(); ++i)
          tck_mask[i] = value_type(invert ? (tck_scalar[i] <= threshold) : (tck_scalar[i] > threshold));
        results.push_back(tck_mask);
      }
    } // reader closed here; safe to write to the same TRX

    if (trx_out) {
      DWI::Tractography::TRX::TRXScalarWriter writer(output_path, field_out);
      for (const auto &s : results)
        writer(s);
      writer.finalize();
    } else {
      // TSF input mode was TRX but output is TSF — need properties from somewhere;
      // use empty properties (no timestamp/count matching required for TRX→TSF).
      DWI::Tractography::Properties props;
      DWI::Tractography::ScalarWriter<value_type> writer(output_path, props);
      for (const auto &s : results)
        writer(s);
    }

  } else {
    // TSF input mode
    DWI::Tractography::Properties properties;
    DWI::Tractography::ScalarReader<value_type> reader(input_path, properties);

    if (trx_out) {
      std::vector<DWI::Tractography::TrackScalar<value_type>> results;
      while (reader(tck_scalar)) {
        tck_mask.set_index(tck_scalar.get_index());
        tck_mask.resize(tck_scalar.size());
        for (size_t i = 0; i < tck_scalar.size(); ++i)
          tck_mask[i] = value_type(invert ? (tck_scalar[i] <= threshold) : (tck_scalar[i] > threshold));
        results.push_back(tck_mask);
      }
      DWI::Tractography::TRX::TRXScalarWriter writer(output_path, field_out);
      for (const auto &s : results)
        writer(s);
      writer.finalize();
    } else {
      DWI::Tractography::ScalarWriter<value_type> writer(output_path, properties);
      while (reader(tck_scalar)) {
        tck_mask.set_index(tck_scalar.get_index());
        tck_mask.resize(tck_scalar.size());
        for (size_t i = 0; i < tck_scalar.size(); ++i)
          tck_mask[i] = value_type(invert ? (tck_scalar[i] <= threshold) : (tck_scalar[i] > threshold));
        writer(tck_mask);
      }
    }
  }
}
