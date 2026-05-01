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

#include "fixel/correspondence/mapping.h"

#include <string>

#include "file/npz.h"

namespace MR::Fixel::Correspondence {

Mapping::Mapping(const uint32_t source_fixels, const uint32_t target_fixels)
    : source_fixels(source_fixels), target_fixels(target_fixels), M(target_fixels, std::vector<Entry>()) {}

Mapping::Mapping(std::string_view path) { load(path); }

namespace {

template <typename T>
void validate_npy_1d(File::NPY::ReadInfo info, std::string_view entry_name, std::string_view npz_path) {
  if (info.shape.size() != 1)
    throw Exception("Expected 1D array in entry \"" + std::string(entry_name) + "\"" + //
                    " in \"" + std::string(npz_path) + "\"");                          //
  if (info.data_type != DataType::from<T>())
    throw Exception("Unexpected data type in entry \"" + std::string(entry_name) + "\"" + //
                    " in \"" + std::string(npz_path) + "\"");                             //
  if (!info.data_type.is_byte_order_native())
    throw Exception("Non-native byte order in entry \"" + std::string(entry_name) + "\"" + //
                    " in \"" + std::string(npz_path) + "\"");                              //
}

} // namespace

void Mapping::load(std::string_view npz_path, const bool import_inverse) {
  const std::string dir_string = import_inverse ? "inverse" : "forward";
  const std::string converse_string = import_inverse ? "forward" : "inverse";
  const std::string indptr_name = "indptr_" + dir_string + ".npy";
  const std::string indices_name = "indices_" + dir_string + ".npy";
  const std::string data_name = "data_" + dir_string + ".npy";
  const std::string converse_indptr_name = "indptr_" + converse_string + ".npy";

  int errcode = 0;
  zip_t *archive = zip_open(std::string(npz_path).c_str(), ZIP_RDONLY, &errcode);
  if (archive == nullptr) {
    zip_error_t error;
    zip_error_init_with_code(&error, errcode);
    const std::string message = zip_error_strerror(&error);
    zip_error_fini(&error);
    throw Exception("Failed to open NPZ file \"" + std::string(npz_path) + "\": " + message);
  }

  const std::vector<uint8_t> indptr_buf = File::NPZ::read_entry(archive, indptr_name, npz_path);
  const std::vector<uint8_t> indices_buf = File::NPZ::read_entry(archive, indices_name, npz_path);
  const std::vector<uint8_t> data_buf = File::NPZ::read_entry(archive, data_name, npz_path);
  const std::vector<uint8_t> converse_buf = File::NPZ::read_entry(archive, converse_indptr_name, npz_path);

  zip_close(archive);

  const File::NPY::ReadInfo indptr_info = File::NPZ::parse_1d_header_from_buffer(indptr_buf, indptr_name);
  const File::NPY::ReadInfo indices_info = File::NPZ::parse_1d_header_from_buffer(indices_buf, indices_name);
  const File::NPY::ReadInfo data_info = File::NPZ::parse_1d_header_from_buffer(data_buf, data_name);
  const File::NPY::ReadInfo converse_info = File::NPZ::parse_1d_header_from_buffer(converse_buf, converse_indptr_name);

  validate_npy_1d<uint32_t>(indptr_info, indptr_name, npz_path);
  validate_npy_1d<uint32_t>(indices_info, indices_name, npz_path);
  validate_npy_1d<float>(data_info, data_name, npz_path);
  validate_npy_1d<uint32_t>(converse_info, converse_indptr_name, npz_path);

  if (indices_info.shape[0] != data_info.shape[0])
    throw Exception("Size mismatch between indices and data arrays in NPZ file \"" + std::string(npz_path) + "\"");

  const uint32_t N = static_cast<uint32_t>(indptr_info.shape[0]) - 1;
  M.assign(N, std::vector<Entry>());

  const uint32_t *indptr = reinterpret_cast<const uint32_t *>(indptr_buf.data() + indptr_info.data_offset);
  const uint32_t *indices = reinterpret_cast<const uint32_t *>(indices_buf.data() + indices_info.data_offset);
  const float *data = reinterpret_cast<const float *>(data_buf.data() + data_info.data_offset);

  for (uint32_t i = 0; i != N; ++i) {
    const uint32_t begin = indptr[i];
    const uint32_t end = indptr[i + 1];
    M[i].reserve(end - begin);
    for (uint32_t j = begin; j != end; ++j)
      M[i].push_back({indices[j], data[j]});
  }

  source_fixels = static_cast<uint32_t>(converse_info.shape[0]) - 1;
  target_fixels = N;
}

void Mapping::save(std::string_view npz_path) const {
  const Mapping inv = inverse();

  // Build CSR arrays for forward mapping
  const uint32_t fwd_N = static_cast<uint32_t>(M.size());
  uint32_t fwd_total = 0;
  for (const auto &row : M)
    fwd_total += static_cast<uint32_t>(row.size());

  std::vector<uint32_t> fwd_indptr(fwd_N + 1);
  std::vector<uint32_t> fwd_indices(fwd_total);
  std::vector<float> fwd_data(fwd_total);
  {
    uint32_t offset = 0;
    size_t k = 0;
    for (size_t i = 0; i != fwd_N; ++i) {
      fwd_indptr[i] = offset;
      for (const auto &e : M[i]) {
        fwd_indices[k] = e.index;
        fwd_data[k] = e.weight;
        ++k;
      }
      offset += static_cast<uint32_t>(M[i].size());
    }
    fwd_indptr[fwd_N] = offset;
  }

  // Build CSR arrays for inverse mapping
  const uint32_t inv_N = static_cast<uint32_t>(inv.M.size());
  uint32_t inv_total = 0;
  for (const auto &row : inv.M)
    inv_total += static_cast<uint32_t>(row.size());

  std::vector<uint32_t> inv_indptr(inv_N + 1);
  std::vector<uint32_t> inv_indices(inv_total);
  std::vector<float> inv_data(inv_total);
  {
    uint32_t offset = 0;
    size_t k = 0;
    for (size_t i = 0; i != inv_N; ++i) {
      inv_indptr[i] = offset;
      for (const auto &e : inv.M[i]) {
        inv_indices[k] = e.index;
        inv_data[k] = e.weight;
        ++k;
      }
      offset += static_cast<uint32_t>(inv.M[i].size());
    }
    inv_indptr[inv_N] = offset;
  }

  File::NPZ::Writer writer(npz_path);
  writer.add_1d("indptr_forward.npy", fwd_indptr.data(), fwd_indptr.size());
  writer.add_1d("indices_forward.npy", fwd_indices.data(), fwd_indices.size());
  writer.add_1d("data_forward.npy", fwd_data.data(), fwd_data.size());
  writer.add_1d("indptr_inverse.npy", inv_indptr.data(), inv_indptr.size());
  writer.add_1d("indices_inverse.npy", inv_indices.data(), inv_indices.size());
  writer.add_1d("data_inverse.npy", inv_data.data(), inv_data.size());
  writer.close();
}

Mapping Mapping::inverse() const {
  std::vector<float> target_weight_sum(target_fixels, 0.0f);
  for (uint32_t t = 0; t != target_fixels; ++t)
    for (const auto &e : M[t])
      target_weight_sum[t] += e.weight;

  Mapping inv(target_fixels, source_fixels);
  for (uint32_t t = 0; t != target_fixels; ++t)
    for (const auto &e : M[t])
      inv.M[e.index].push_back({t, target_weight_sum[t] > 0.0f ? e.weight / target_weight_sum[t] : 0.0f});
  return inv;
}

} // namespace MR::Fixel::Correspondence
