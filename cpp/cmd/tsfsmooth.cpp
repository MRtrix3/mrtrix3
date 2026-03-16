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

constexpr float default_smoothing = 4.0F;

// clang-format off
void usage() {

  AUTHOR = "David Raffelt (david.raffelt@florey.edu.au)";

  SYNOPSIS = "Gaussian filter a track scalar file";

  DESCRIPTION
  + "Accepts .tsf track scalar files or TRX dpv fields as input and output. "
    "Use -field_in / -field_out to name the dpv fields when the corresponding "
    "argument is a TRX file.";

  ARGUMENTS
  + Argument ("input",  "the input track scalar file or TRX tractogram").type_file_in()
  + Argument ("output", "the output track scalar file or TRX tractogram").type_file_out();

  OPTIONS
  + Option ("stdev", "apply Gaussian smoothing with the specified standard deviation."
                     " The standard deviation is defined in units of track points"
                     " (default: " + str(default_smoothing, 2) + ")")
    + Argument ("sigma").type_float(1e-6)

  + Option ("field_in", "dpv field name to read from input TRX file")
    + Argument ("name").type_text()

  + Option ("field_out", "dpv field name to write to output TRX file "
                         "(output TRX must already exist; use the same path as input to add in-place)")
    + Argument ("name").type_text();

}
// clang-format on

using value_type = float;

static void smooth_scalar(DWI::Tractography::TrackScalar<value_type> &out,
                          const DWI::Tractography::TrackScalar<value_type> &in,
                          const std::vector<float> &kernel,
                          const int floor_radius) {
  out.set_index(in.get_index());
  out.resize(in.size());
  const float radius = (static_cast<float>(kernel.size()) - 1.0F) / 2.0F;
  for (int i = 0; i < static_cast<int>(in.size()); ++i) {
    float norm_factor = 0.0F;
    float value = 0.0F;
    for (int k = -floor_radius; k <= floor_radius; ++k) {
      if (i + k >= 0 && i + k < static_cast<int>(in.size())) {
        value += kernel[static_cast<size_t>(k + static_cast<int>(radius))] * in[static_cast<size_t>(i + k)];
        norm_factor += kernel[static_cast<size_t>(k + static_cast<int>(radius))];
      }
    }
    out[static_cast<size_t>(i)] = value / norm_factor;
  }
}

void run() {
  const std::string input_path(argument[0]);
  const std::string output_path(argument[1]);

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

  const float stdev = get_option_value("stdev", default_smoothing);
  std::vector<float> kernel(static_cast<size_t>(2 * std::ceil(2.5F * stdev) + 1), 0.0F);
  float norm_factor = 0.0F;
  const float radius = (static_cast<float>(kernel.size()) - 1.0F) / 2.0F;
  const int floor_radius = static_cast<int>(std::floor(radius));
  for (size_t c = 0; c < kernel.size(); ++c) {
    kernel[c] = std::exp(-(static_cast<float>(c) - radius) * (static_cast<float>(c) - radius) / (2.0F * stdev * stdev));
    norm_factor += kernel[c];
  }
  for (size_t c = 0; c < kernel.size(); ++c)
    kernel[c] /= norm_factor;

  DWI::Tractography::TrackScalar<value_type> tck_scalar, tck_smoothed;

  if (trx_in) {
    std::vector<DWI::Tractography::TrackScalar<value_type>> results;
    {
      DWI::Tractography::TRX::TRXScalarReader reader(input_path, field_in);
      results.reserve(reader.num_streamlines());
      while (reader(tck_scalar)) {
        smooth_scalar(tck_smoothed, tck_scalar, kernel, floor_radius);
        results.push_back(tck_smoothed);
      }
    } // reader closed; safe to write to the same TRX

    if (trx_out) {
      DWI::Tractography::TRX::TRXScalarWriter writer(output_path, field_out);
      for (const auto &s : results)
        writer(s);
      writer.finalize();
    } else {
      DWI::Tractography::Properties props;
      DWI::Tractography::ScalarWriter<value_type> writer(output_path, props);
      for (const auto &s : results)
        writer(s);
    }

  } else {
    DWI::Tractography::Properties properties;
    DWI::Tractography::ScalarReader<value_type> reader(input_path, properties);

    if (trx_out) {
      std::vector<DWI::Tractography::TrackScalar<value_type>> results;
      while (reader(tck_scalar)) {
        smooth_scalar(tck_smoothed, tck_scalar, kernel, floor_radius);
        results.push_back(tck_smoothed);
      }
      DWI::Tractography::TRX::TRXScalarWriter writer(output_path, field_out);
      for (const auto &s : results)
        writer(s);
      writer.finalize();
    } else {
      DWI::Tractography::ScalarWriter<value_type> writer(output_path, properties);
      while (reader(tck_scalar)) {
        smooth_scalar(tck_smoothed, tck_scalar, kernel, floor_radius);
        writer(tck_smoothed);
      }
    }
  }
}
