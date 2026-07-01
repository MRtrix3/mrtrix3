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

#include "dwi/tractography/sidecar_export.h"

#include <cstddef>
#include <random>
#include <system_error>

#include "app.h"
#include "datatype.h"
#include "exception.h"
#include "file/matrix.h"
#include "mrtrix.h"
#include "progressbar.h"
#include "raw.h"
#include "signal_handler.h"

#include "dwi/tractography/field_registry.h"
#include "dwi/tractography/formats/base.h"
#include "dwi/tractography/formats/list.h"
#include "dwi/tractography/sidecar_value.h"
#include "dwi/tractography/tractogram.h"

namespace MR::DWI::Tractography {

namespace {

//! \brief serialise a per-streamline value column to little-endian float32 bytes.
std::vector<std::byte> serialise_dps_float32(const Eigen::Array<default_type, Eigen::Dynamic, 1> &values) {
  std::vector<std::byte> bytes(static_cast<size_t>(values.size()) * sizeof(float));
  for (Eigen::Index i = 0; i != values.size(); ++i)
    Raw::store_LE<float>(static_cast<float>(values[i]), bytes.data() + static_cast<size_t>(i) * sizeof(float));
  return bytes;
}

//! \brief write a copy of \a input_path carrying \a values as a new dps field.
/*! Streams every input streamline through to \a output_path with its value
 * slotted into a new Float32 per-streamline field \a field_name, preserving any
 * sidecar data the input already carries. The streamline count must match
 * \a values exactly. */
void stream_copy_with_field(const std::filesystem::path &input_path,
                            const std::filesystem::path &output_path,
                            std::string_view field_name,
                            const Eigen::Array<default_type, Eigen::Dynamic, 1> &values,
                            const std::vector<std::pair<std::string, std::string>> &extra_properties) {
  Properties properties;
  auto input = Tractogram<float>::open(input_path, properties);
  for (const auto &keyval : extra_properties)
    properties[keyval.first] = keyval.second;

  // Declare the output field set from the input registry plus the new field, so
  //   the input's own sidecar data passes through unchanged and the values ride
  //   alongside as a named per-streamline (dps) field.
  FieldRegistry registry = input.fields();
  if (registry.ordinal(field_name, FieldRole::DPS).has_value())
    throw Exception("input tractogram \"" + input_path.string() + "\"" +
                    " already carries a per-streamline field named \"" + std::string(field_name) + "\"");
  const size_t ordinal =
      registry.add({std::string(field_name), FieldRole::DPS, DataType::Float32, 1, FieldSource::Internal, 0});

  auto output = Tractogram<float>::create(output_path, properties, registry);

  TractogramItem<float> item;
  Eigen::Index counter = 0;
  ProgressBar progress("writing tractogram with embedded \"" + std::string(field_name) + "\" field", values.size());
  while (input.read(item)) {
    if (counter == values.size())
      throw Exception("input tractogram \"" + input_path.string() + "\" contains more streamlines than the " +
                      str(values.size()) + " supplied values");
    item.dps.resize(registry.dps_count());
    ScalarOrVector<float> value(1);
    value(0, 0) = static_cast<float>(values[counter++]);
    item.dps[ordinal] = make_dps(std::move(value));
    output.write(item);
    ++progress;
  }
  if (counter != values.size())
    throw Exception("input tractogram \"" + input_path.string() + "\" contains only " + str(counter) +
                    " streamlines but " + str(values.size()) + " values were supplied");
}

//! \brief whether \a dataset already carries a per-streamline field of this name.
/*! Opens the dataset for reading (header/member directory only; no streamline
 * payload) and inspects the field registry the handler populates. */
bool dps_field_present(const std::filesystem::path &dataset, std::string_view field_name) {
  Properties properties;
  auto existing = Tractogram<float>::open(dataset, properties);
  return existing.fields().ordinal(field_name, FieldRole::DPS).has_value();
}

} // namespace

std::filesystem::path make_sibling_path(const std::filesystem::path &dest) {
  std::filesystem::path dir = dest.parent_path();
  if (dir.empty())
    dir = ".";
  const std::string stem = dest.stem().string();
  const std::string ext = dest.extension().string();
  thread_local std::mt19937 rng{std::random_device{}()};
  std::uniform_int_distribution<int> dist(0, 35);
  for (int attempt = 0; attempt != 4096; ++attempt) {
    std::string token(8, '\0');
    for (char &c : token) {
      const int v = dist(rng);
      c = (v < 10) ? static_cast<char>('0' + v) : static_cast<char>('a' + (v - 10));
    }
    std::filesystem::path candidate = dir / (stem + "-mrtrixtmp-" + token + ext);
    std::error_code ec;
    if (!std::filesystem::exists(candidate, ec))
      return candidate;
  }
  throw Exception("unable to allocate a temporary file beside \"" + dest.string() + "\"");
}

SidecarDestination classify_sidecar_destination(const Formats::Base &handler, const std::filesystem::path &dataset) {
  std::error_code ec;
  if (!std::filesystem::exists(dataset, ec))
    return SidecarDestination::CreateDataset;
  if (handler.capabilities.sidecar_data == Formats::SidecarData::Append)
    return SidecarDestination::AppendInPlace;
  return SidecarDestination::RewriteDataset;
}

void export_sidecar_column(const SidecarReference &reference,
                           const std::filesystem::path &input_path,
                           const Eigen::Array<default_type, Eigen::Dynamic, 1> &values,
                           const std::string_view default_field_name,
                           const std::vector<std::pair<std::string, std::string>> &extra_properties) {
  const Formats::Base *const handler = Formats::get_handler(reference.dataset);

  // A bare path that no tractography format recognises is a standalone numerical
  //   vector (one value per streamline). A qualified "DATASET::NAME" reference to
  //   such a path is meaningless and is rejected.
  if (handler == nullptr) {
    if (reference.is_qualified())
      throw Exception("sidecar reference names a field \"" + *reference.name + "\"" +
                      " within tractography dataset \"" + reference.dataset.string() + "\"," +
                      " but that path is not a recognised tractography format");
    File::Matrix::save_vector(values, reference.dataset);
    return;
  }

  const std::string field_name = reference.is_qualified() ? *reference.name : std::string(default_field_name);

  // The format must be able to carry per-streamline sidecar data at all.
  if (handler->capabilities.sidecar_data == Formats::SidecarData::Unsupported)
    throw Exception("output tractography format \"" + handler->description + "\"" +
                    " cannot carry per-streamline sidecar data," + " so the values cannot be embedded into \"" +
                    reference.dataset.string() + "\"" +
                    " (use a format such as \".trx\", or write a standalone weights file)");

  switch (classify_sidecar_destination(*handler, reference.dataset)) {
  case SidecarDestination::CreateDataset:
    // The destination does not yet exist: write a fresh augmented copy of the input.
    stream_copy_with_field(input_path, reference.dataset, field_name, values, extra_properties);
    return;

  case SidecarDestination::AppendInPlace:
    // The format can take a new sidecar member in place: write it directly without
    //   rewriting the streamline data. Overwrite permission is required only if a
    //   field of this name is already present.
    if (dps_field_present(reference.dataset, field_name))
      App::check_overwrite(reference.dataset);
    {
      const FieldDescriptor descriptor{field_name, FieldRole::DPS, DataType::Float32, 1, FieldSource::Internal, 0};
      handler->append_sidecar(reference.dataset, descriptor, serialise_dps_float32(values));
    }
    return;

  case SidecarDestination::RewriteDataset:
    // The format must be rewritten to add a field: require overwrite permission,
    //   write the augmented dataset beside the destination, then atomically replace
    //   it. (A rewritten dataset is emitted in the standard fresh-write form; e.g. a
    //   compressed TRX archive is re-emitted per the TRXArchiveCompression config.)
    App::check_overwrite(reference.dataset);
    const std::filesystem::path scratch = make_sibling_path(reference.dataset);
    SignalHandler::mark_file_for_deletion(scratch);
    try {
      stream_copy_with_field(input_path, scratch, field_name, values, extra_properties);
      std::filesystem::rename(scratch, reference.dataset);
    } catch (...) {
      std::error_code cleanup_ec;
      std::filesystem::remove(scratch, cleanup_ec);
      throw;
    }
    SignalHandler::unmark_file_for_deletion(scratch);
    return;
  }
}

} // namespace MR::DWI::Tractography
