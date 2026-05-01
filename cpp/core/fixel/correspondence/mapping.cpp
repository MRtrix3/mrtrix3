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

Mapping::Mapping(const index_type source_fixels, const index_type target_fixels)
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

  validate_npy_1d<npz_index_type>(indptr_info, indptr_name, npz_path);
  validate_npy_1d<npz_index_type>(indices_info, indices_name, npz_path);
  validate_npy_1d<npz_value_type>(data_info, data_name, npz_path);
  validate_npy_1d<npz_index_type>(converse_info, converse_indptr_name, npz_path);

  if (indices_info.shape[0] != data_info.shape[0])
    throw Exception("Size mismatch between indices and data arrays in NPZ file \"" + std::string(npz_path) + "\"");

  const index_type N = static_cast<index_type>(indptr_info.shape[0]) - 1;
  M.assign(N, std::vector<Entry>());

  const npz_index_type *indptr = reinterpret_cast<const npz_index_type *>(indptr_buf.data() + indptr_info.data_offset);
  const npz_index_type *indices =
      reinterpret_cast<const npz_index_type *>(indices_buf.data() + indices_info.data_offset);
  const npz_value_type *data = reinterpret_cast<const npz_value_type *>(data_buf.data() + data_info.data_offset);

  for (index_type i = 0; i != N; ++i) {
    const npz_index_type begin = indptr[i];
    const npz_index_type end = indptr[i + 1];
    M[i].reserve(end - begin);
    for (npz_index_type j = begin; j != end; ++j)
      M[i].push_back({indices[j], data[j]});
  }

  source_fixels = static_cast<index_type>(converse_info.shape[0]) - 1;
  target_fixels = N;
}

void Mapping::save(std::string_view npz_path) const {
  const Mapping inv = inverse();

  // Build CSR arrays for forward mapping
  const index_type fwd_N = static_cast<index_type>(M.size());
  npz_index_type fwd_total = 0;
  for (const auto &row : M)
    fwd_total += static_cast<npz_index_type>(row.size());

  std::vector<npz_index_type> fwd_indptr(fwd_N + 1);
  std::vector<npz_index_type> fwd_indices(fwd_total);
  std::vector<npz_value_type> fwd_data(fwd_total);
  {
    npz_index_type offset = 0;
    size_t k = 0;
    for (index_type i = 0; i != fwd_N; ++i) {
      fwd_indptr[i] = offset;
      for (const auto &e : M[i]) {
        fwd_indices[k] = e.index;
        fwd_data[k] = e.weight;
        ++k;
      }
      offset += static_cast<npz_index_type>(M[i].size());
    }
    fwd_indptr[fwd_N] = offset;
  }

  // Build CSR arrays for inverse mapping
  const index_type inv_N = static_cast<index_type>(inv.M.size());
  npz_index_type inv_total = 0;
  for (const auto &row : inv.M)
    inv_total += static_cast<npz_index_type>(row.size());

  std::vector<npz_index_type> inv_indptr(inv_N + 1);
  std::vector<npz_index_type> inv_indices(inv_total);
  std::vector<npz_value_type> inv_data(inv_total);
  {
    npz_index_type offset = 0;
    size_t k = 0;
    for (index_type i = 0; i != inv_N; ++i) {
      inv_indptr[i] = offset;
      for (const auto &e : inv.M[i]) {
        inv_indices[k] = e.index;
        inv_data[k] = e.weight;
        ++k;
      }
      offset += static_cast<npz_index_type>(inv.M[i].size());
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
  for (index_type t = 0; t != target_fixels; ++t)
    for (const auto &e : M[t])
      target_weight_sum[t] += e.weight;

  Mapping inv(target_fixels, source_fixels);
  for (index_type t = 0; t != target_fixels; ++t)
    for (const auto &e : M[t])
      inv.M[e.index].push_back({t, target_weight_sum[t] > 0.0f ? e.weight / target_weight_sum[t] : 0.0f});
  return inv;
}

} // namespace MR::Fixel::Correspondence
