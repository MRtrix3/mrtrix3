/* Copyright (c) 2008-2017 the MRtrix3 contributors.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, you can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * MRtrix is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * For more details, see http://www.mrtrix.org/.
 */

#include "fixel/correspondence/mapping.h"

#include <string>

#include "file/npy.h"
#include "file/path.h"
#include "file/utils.h"

namespace MR::Fixel::Correspondence {

Mapping::Mapping(const uint32_t source_fixels, const uint32_t target_fixels)
    : source_fixels(source_fixels), target_fixels(target_fixels), M(target_fixels, std::vector<Entry>()) {}

Mapping::Mapping(std::string_view directory) { load(directory); }

namespace {

template <typename T> File::NPY::ReadInfo load_npy_1d(std::string_view path) {
  File::NPY::ReadInfo info = File::NPY::read_header(path);
  if (info.shape.size() != 1)
    throw Exception("Expected 1D array in fixel correspondence file \"" + path + "\"");
  if (info.data_type != DataType::from<T>())
    throw Exception("Unexpected data type in fixel correspondence file \"" + path + "\"");
  if (!info.data_type.is_byte_order_native())
    throw Exception("Non-native byte order in fixel correspondence file \"" + path + "\"");
  return info;
}

} // namespace

void Mapping::load(std::string_view directory, const bool import_inverse) {
  const std::string dir_string = import_inverse ? "inverse" : "forward";
  const std::string indptr_path = Path::join(directory, "indptr_" + dir_string + ".npy");
  const std::string indices_path = Path::join(directory, "indices_" + dir_string + ".npy");
  const std::string data_path = Path::join(directory, "data_" + dir_string + ".npy");
  const std::string converse_indptr_path =
      Path::join(directory, std::string("indptr_") + (import_inverse ? "forward" : "inverse") + ".npy");

  const File::NPY::ReadInfo indptr_info = load_npy_1d<uint32_t>(indptr_path);
  const File::NPY::ReadInfo indices_info = load_npy_1d<uint32_t>(indices_path);
  const File::NPY::ReadInfo data_info = load_npy_1d<float>(data_path);
  const File::NPY::ReadInfo converse_info = load_npy_1d<uint32_t>(converse_indptr_path);

  if (indices_info.shape[0] != data_info.shape[0])
    throw Exception("Size mismatch between indices and data arrays in fixel correspondence directory \"" + directory +
                    "\"");

  const uint32_t N = static_cast<uint32_t>(indptr_info.shape[0]) - 1;
  M.assign(N, std::vector<Entry>());

  {
    File::MMap indptr_mmap({indptr_path, indptr_info.data_offset}, false);
    File::MMap indices_mmap({indices_path, indices_info.data_offset}, false);
    File::MMap data_mmap({data_path, data_info.data_offset}, false);
    const Eigen::Map<const Eigen::Array<uint32_t, Eigen::Dynamic, 1>> indptr_map(
        reinterpret_cast<const uint32_t *>(indptr_mmap.address()), indptr_info.shape[0]);
    const Eigen::Map<const Eigen::Array<uint32_t, Eigen::Dynamic, 1>> indices_map(
        reinterpret_cast<const uint32_t *>(indices_mmap.address()), indices_info.shape[0]);
    const Eigen::Map<const Eigen::Array<float, Eigen::Dynamic, 1>> data_map(
        reinterpret_cast<const float *>(data_mmap.address()), data_info.shape[0]);
    for (uint32_t i = 0; i != N; ++i) {
      const uint32_t begin = indptr_map(i);
      const uint32_t end = indptr_map(i + 1);
      M[i].reserve(end - begin);
      for (uint32_t j = begin; j != end; ++j)
        M[i].push_back({indices_map(j), data_map(j)});
    }
  }

  source_fixels = static_cast<uint32_t>(converse_info.shape[0]) - 1;
  target_fixels = N;
}

void Mapping::save(std::string_view directory) const {
  File::mkdir(directory);
  save(directory, false);
  save(directory, true);
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

void Mapping::save(std::string_view directory, const bool export_inverse) const {
  Mapping inv(0, 0);
  if (export_inverse)
    inv = inverse();
  const std::vector<std::vector<Entry>> &data(export_inverse ? inv.M : M);
  const std::string dir_string = export_inverse ? "inverse" : "forward";
  const std::string indptr_path = Path::join(directory, "indptr_" + dir_string + ".npy");
  const std::string indices_path = Path::join(directory, "indices_" + dir_string + ".npy");
  const std::string data_path = Path::join(directory, "data_" + dir_string + ".npy");

  uint32_t total_entries = 0;
  for (const auto &row : data)
    total_entries += static_cast<uint32_t>(row.size());

  // Write CSR index pointer array: N+1 uint32 values, indptr[i] is the offset
  //   of entry i in the indices array; indptr[N] holds the total entry count
  {
    File::NPY::WriteInfo indptr_write =
        File::NPY::prepare_ND_write(indptr_path, DataType::from<uint32_t>(), {data.size() + 1});
    uint32_t *ptr = reinterpret_cast<uint32_t *>(indptr_write.mmap->address());
    uint32_t offset = 0;
    for (size_t i = 0; i != data.size(); ++i) {
      ptr[i] = offset;
      offset += static_cast<uint32_t>(data[i].size());
    }
    ptr[data.size()] = offset;
  }

  {
    File::NPY::WriteInfo indices_write =
        File::NPY::prepare_ND_write(indices_path, DataType::from<uint32_t>(), {total_entries});
    uint32_t *ptr = reinterpret_cast<uint32_t *>(indices_write.mmap->address());
    size_t k = 0;
    for (const auto &row : data)
      for (const auto &e : row)
        ptr[k++] = e.index;
  }

  {
    File::NPY::WriteInfo data_write = File::NPY::prepare_ND_write(data_path, DataType::from<float>(), {total_entries});
    float *ptr = reinterpret_cast<float *>(data_write.mmap->address());
    size_t k = 0;
    for (const auto &row : data)
      for (const auto &e : row)
        ptr[k++] = e.weight;
  }
}

} // namespace MR::Fixel::Correspondence
