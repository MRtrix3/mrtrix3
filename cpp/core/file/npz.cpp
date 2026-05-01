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

#include "file/npz.h"

#include "raw.h"

namespace MR::File::NPZ {

NPY::ReadInfo parse_1d_header_from_buffer(const std::vector<uint8_t> &buffer, std::string_view context) {
  if (buffer.size() < 10)
    throw Exception("Buffer too small to be a valid NPY file" +
                    std::string(context.empty() ? "" : " in " + std::string(context)));

  for (size_t i = 0; i != 6; ++i) {
    if (buffer[i] != static_cast<uint8_t>(NPY::magic_string[i]))
      throw Exception("Invalid NPY magic string" + std::string(context.empty() ? "" : " in " + std::string(context)));
  }

  const uint8_t major_version = buffer[6];
  uint32_t header_len = 0;
  size_t leadin_size = 0;
  if (major_version == 1) {
    const uint16_t header_len_short = ByteOrder::LE(*reinterpret_cast<const uint16_t *>(buffer.data() + 8));
    header_len = static_cast<uint32_t>(header_len_short);
    leadin_size = 10;
  } else if (major_version == 2) {
    header_len = ByteOrder::LE(*reinterpret_cast<const uint32_t *>(buffer.data() + 8));
    leadin_size = 12;
  } else {
    throw Exception("Incompatible NPY major version (" + str(major_version) + ")" +
                    std::string(context.empty() ? "" : " in " + std::string(context)));
  }

  if (buffer.size() < leadin_size + header_len)
    throw Exception("NPY buffer too small for stated header length" +
                    std::string(context.empty() ? "" : " in " + std::string(context)));

  const std::string header_str(reinterpret_cast<const char *>(buffer.data() + leadin_size), header_len);

  NPY::ReadInfo info;
  info.data_offset = static_cast<int64_t>(leadin_size + header_len);

  try {
    info.keyval = NPY::parse_dict(header_str);
  } catch (Exception &e) {
    throw Exception(e, "Error parsing NPY header" + std::string(context.empty() ? "" : " in " + std::string(context)));
  }

  const auto descr_ptr = info.keyval.find("descr");
  if (descr_ptr == info.keyval.end())
    throw Exception("NPY header missing \"descr\" key" +
                    std::string(context.empty() ? "" : " in " + std::string(context)));
  try {
    info.data_type = NPY::descr2datatype(descr_ptr->second);
  } catch (Exception &e) {
    throw Exception(e,
                    "Error parsing NPY data type" + std::string(context.empty() ? "" : " in " + std::string(context)));
  }
  info.keyval.erase(descr_ptr);

  const auto fortran_ptr = info.keyval.find("fortran_order");
  if (fortran_ptr == info.keyval.end())
    throw Exception("NPY header missing \"fortran_order\" key" +
                    std::string(context.empty() ? "" : " in " + std::string(context)));
  info.column_major = to<bool>(fortran_ptr->second);
  info.keyval.erase(fortran_ptr);

  const auto shape_ptr = info.keyval.find("shape");
  if (shape_ptr == info.keyval.end())
    throw Exception("NPY header missing \"shape\" key" +
                    std::string(context.empty() ? "" : " in " + std::string(context)));
  const auto shape_parts = split(strip(strip(shape_ptr->second, "(", true, false), ")", false, true), ",", true);
  if (shape_parts.size() != 1)
    throw Exception("Expected 1D shape in NPY entry" +
                    std::string(context.empty() ? "" : " in " + std::string(context)));
  info.shape.push_back(to<ssize_t>(shape_parts[0]));
  info.keyval.erase(shape_ptr);

  return info;
}

std::vector<uint8_t> read_entry(zip_t *archive, std::string_view entry_name, std::string_view npz_path) {
  const std::string entry_name_str(entry_name);
  const zip_int64_t index = zip_name_locate(archive, entry_name_str.c_str(), 0);
  if (index < 0)
    throw Exception("Entry \"" + entry_name_str + "\" not found in \"" + std::string(npz_path) + "\"");

  zip_stat_t stat;
  if (zip_stat_index(archive, static_cast<zip_uint64_t>(index), 0, &stat) != 0)
    throw Exception("Failed to stat entry \"" + entry_name_str + "\" in \"" + std::string(npz_path) + "\"");

  std::vector<uint8_t> buffer(stat.size);

  zip_file_t *zf = zip_fopen_index(archive, static_cast<zip_uint64_t>(index), 0);
  if (zf == nullptr)
    throw Exception("Failed to open entry \"" + entry_name_str + "\" in \"" + std::string(npz_path) +
                    "\": " + zip_error_strerror(zip_get_error(archive)));

  const zip_int64_t bytes_read = zip_fread(zf, buffer.data(), stat.size);
  zip_fclose(zf);

  if (bytes_read < 0 || static_cast<zip_uint64_t>(bytes_read) != stat.size)
    throw Exception("Failed to read entry \"" + entry_name_str + "\" in \"" + std::string(npz_path) + "\"");

  return buffer;
}

Writer::Writer(std::string_view path) : path_(path), closed_(false) {
  int errcode = 0;
  archive_ = zip_open(path_.c_str(), ZIP_CREATE | ZIP_EXCL, &errcode);
  if (archive_ == nullptr) {
    zip_error_t error;
    zip_error_init_with_code(&error, errcode);
    const std::string message = zip_error_strerror(&error);
    zip_error_fini(&error);
    throw Exception("Failed to create NPZ file \"" + path_ + "\": " + message);
  }
}

Writer::~Writer() {
  if (!closed_) {
    try {
      close();
    } catch (...) {
      zip_discard(archive_);
    }
  }
}

void Writer::close() {
  if (closed_)
    return;
  closed_ = true;
  if (zip_close(archive_) != 0) {
    const std::string message = zip_error_strerror(zip_get_error(archive_));
    zip_discard(archive_);
    throw Exception("Failed to close NPZ file \"" + path_ + "\": " + message);
  }
}

} // namespace MR::File::NPZ
