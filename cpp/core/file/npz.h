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

#pragma once

#include <string>
#include <string_view>
#include <vector>

#include <zip.h>

#include "datatype.h"
#include "exception.h"
#include "file/npy.h"

namespace MR::File::NPZ {

/// @brief Build complete NPY file content for a 1D array into a memory buffer.
template <typename T> std::vector<uint8_t> build_1d_buffer(const T *data, const size_t count) {
  const DataType data_type = DataType::from<T>();
  const std::string descr = NPY::datatype2descr(data_type);
  std::string header =
      std::string("{'descr': '") + descr + "', 'fortran_order': False, 'shape': (" + str(count) + ",), }";
  // Pad to make total leadin (6 magic + 2 version + 2 header_len + header + '\n') a multiple of 16
  const size_t space_count = NPY::alignment - ((header.size() + 11) % NPY::alignment);
  header.append(space_count, ' ');
  header.push_back('\n');
  const uint16_t padded_header_length = static_cast<uint16_t>(header.size());

  std::vector<uint8_t> buffer;
  buffer.reserve(6 + 2 + 2 + padded_header_length + count * sizeof(T));
  for (const char c : NPY::magic_string)
    buffer.push_back(static_cast<uint8_t>(c));
  buffer.push_back(0x01); // major version
  buffer.push_back(0x00); // minor version
  // header length as little-endian uint16
  buffer.push_back(static_cast<uint8_t>(padded_header_length & 0xFF));
  buffer.push_back(static_cast<uint8_t>((padded_header_length >> 8) & 0xFF));
  for (const char c : header)
    buffer.push_back(static_cast<uint8_t>(c));
  const uint8_t *bytes = reinterpret_cast<const uint8_t *>(data);
  buffer.insert(buffer.end(), bytes, bytes + count * sizeof(T));
  return buffer;
}

/// @brief Parse NPY header from a memory buffer; returns ReadInfo with data_offset as byte offset within buffer.
NPY::ReadInfo parse_1d_header_from_buffer(const std::vector<uint8_t> &buffer, std::string_view context);

/// @brief Read a named .npy entry from an open zip archive into a byte buffer.
std::vector<uint8_t> read_entry(zip_t *archive, std::string_view entry_name, std::string_view npz_path);

/// @brief RAII writer for .npz files (uncompressed ZIP archive of .npy entries).
class Writer {
public:
  explicit Writer(std::string_view path);
  ~Writer();
  Writer(const Writer &) = delete;
  Writer &operator=(const Writer &) = delete;

  /// @brief Add a 1D typed array as a named .npy entry (stored uncompressed).
  template <typename T> void add_1d(std::string_view name, const T *data, const size_t count) {
    buffers_.push_back(build_1d_buffer(data, count));
    const std::vector<uint8_t> &buf = buffers_.back();
    zip_source_t *source = zip_source_buffer(archive_, buf.data(), buf.size(), 0);
    const std::string name_str(name);
    if (source == nullptr)
      throw Exception("Failed to create zip source for entry \"" + name_str + "\" in \"" + path_ + "\"");
    const zip_int64_t index = zip_file_add(archive_, name_str.c_str(), source, ZIP_FL_ENC_UTF_8);
    if (index < 0) {
      zip_source_free(source);
      throw Exception("Failed to add entry \"" + name_str + "\" to \"" + path_ +
                      "\": " + zip_error_strerror(zip_get_error(archive_)));
    }
    if (zip_set_file_compression(archive_, static_cast<zip_uint64_t>(index), ZIP_CM_STORE, 0) != 0)
      throw Exception("Failed to set compression for entry \"" + name_str + "\" in \"" + path_ + "\"");
  }

  /// @brief Commit and close the archive; throws on failure. Called automatically by destructor if not already done.
  void close();

private:
  zip_t *archive_;
  std::string path_;
  bool closed_;
  std::vector<std::vector<uint8_t>> buffers_;
};

} // namespace MR::File::NPZ
