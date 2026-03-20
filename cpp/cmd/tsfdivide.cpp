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

  SYNOPSIS = "Divide corresponding values in track scalar files";

  DESCRIPTION
  + "Accepts .tsf track scalar files or TRX dpv fields as input and output. "
    "Use -field1, -field2, and -field_out to name the dpv fields when the "
    "corresponding argument is a TRX file.";

  ARGUMENTS
  + Argument ("input1", "the first input track scalar file or TRX tractogram").type_file_in()
  + Argument ("input2", "the second input track scalar file or TRX tractogram").type_file_in()
  + Argument ("output", "the output track scalar file or TRX tractogram").type_file_out();

  OPTIONS
  + Option ("field1", "dpv field name to read from the first TRX input")
    + Argument ("name").type_text()

  + Option ("field2", "dpv field name to read from the second TRX input")
    + Argument ("name").type_text()

  + Option ("field_out", "dpv field name to write to the output TRX file "
                         "(output TRX must already exist; use the same path as an input to add in-place)")
    + Argument ("name").type_text();

}
// clang-format on

using value_type = float;

// Load all scalars from a TRX dpv field or TSF file into a vector of TrackScalars
static std::vector<DWI::Tractography::TrackScalar<value_type>> load_scalars(const std::string &path,
                                                                            const std::string &field) {
  std::vector<DWI::Tractography::TrackScalar<value_type>> out;
  DWI::Tractography::TrackScalar<value_type> scalar;
  if (DWI::Tractography::TRX::is_trx(path)) {
    DWI::Tractography::TRX::TRXScalarReader reader(path, field);
    out.reserve(reader.num_streamlines());
    while (reader(scalar))
      out.push_back(scalar);
  } else {
    DWI::Tractography::Properties props;
    DWI::Tractography::ScalarReader<value_type> reader(path, props);
    while (reader(scalar))
      out.push_back(scalar);
  }
  return out;
}

void run() {
  const std::string path1(argument[0]);
  const std::string path2(argument[1]);
  const std::string path_out(argument[2]);

  const bool trx1 = DWI::Tractography::TRX::is_trx(path1);
  const bool trx2 = DWI::Tractography::TRX::is_trx(path2);
  const bool trx_out = DWI::Tractography::TRX::is_trx(path_out);

  auto field1_opt = get_options("field1");
  auto field2_opt = get_options("field2");
  auto field_out_opt = get_options("field_out");

  if (trx1 && field1_opt.empty())
    throw Exception("Use -field1 to specify the dpv field when input1 is a TRX file");
  if (trx2 && field2_opt.empty())
    throw Exception("Use -field2 to specify the dpv field when input2 is a TRX file");
  if (trx_out && field_out_opt.empty())
    throw Exception("Use -field_out to specify the dpv field when the output is a TRX file");
  if (!trx1 && !field1_opt.empty())
    WARN("-field1 is ignored for non-TRX input1");
  if (!trx2 && !field2_opt.empty())
    WARN("-field2 is ignored for non-TRX input2");
  if (!trx_out && !field_out_opt.empty())
    WARN("-field_out is ignored for non-TRX output");

  const std::string field1 = trx1 ? std::string(field1_opt[0][0]) : "";
  const std::string field2 = trx2 ? std::string(field2_opt[0][0]) : "";
  const std::string field_out = trx_out ? std::string(field_out_opt[0][0]) : "";

  // Load both inputs eagerly (enables in-place output on same path as input)
  const auto scalars1 = load_scalars(path1, field1);
  const auto scalars2 = load_scalars(path2, field2);

  if (scalars1.size() != scalars2.size())
    WARN("Input files have different streamline counts (" + str(scalars1.size()) + " vs " + str(scalars2.size()) +
         "); extra streamlines in the longer file will be ignored");

  const size_t n = std::min(scalars1.size(), scalars2.size());
  std::vector<DWI::Tractography::TrackScalar<value_type>> results;
  results.reserve(n);

  for (size_t i = 0; i < n; ++i) {
    const auto &s1 = scalars1[i];
    const auto &s2 = scalars2[i];
    if (s1.size() != s2.size())
      throw Exception("track scalar length mismatch at streamline index " + str(s1.get_index()));
    DWI::Tractography::TrackScalar<value_type> out(s1.size());
    out.set_index(s1.get_index());
    for (size_t j = 0; j < s1.size(); ++j)
      out[j] = (s2[j] == value_type(0)) ? value_type(0) : s1[j] / s2[j];
    results.push_back(std::move(out));
  }

  if (trx_out) {
    DWI::Tractography::TRX::TRXScalarWriter writer(path_out, field_out);
    for (const auto &s : results)
      writer(s);
    writer.finalize();
  } else {
    // Use properties from input1 if it is a TSF, otherwise empty
    DWI::Tractography::Properties props;
    if (!trx1) {
      DWI::Tractography::ScalarReader<value_type> tmp(path1, props);
    }
    DWI::Tractography::ScalarWriter<value_type> writer(path_out, props);
    for (const auto &s : results)
      writer(s);
  }
}
