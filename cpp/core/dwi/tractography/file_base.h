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

#include <filesystem>
#include <iomanip>
#include <ios>
#include <map>
#include <set>
#include <string_view>

#include "dwi/tractography/properties.h"
#include "exception.h"
#include "file/key_value.h"
#include "file/ofstream.h"
#include "file/path.h"
#include "types.h"

namespace MR::DWI::Tractography {

//! \cond skip
class ReaderBase {
public:
  ReaderBase() = default;
  ~ReaderBase() {
    if (in.is_open())
      in.close();
  }

  void open(const std::filesystem::path &file, std::string_view type, Properties &properties);

  void close() { in.close(); }

protected:
  std::ifstream in;
  DataType dtype;
  uint64_t current_index{0};
};

template <typename ValueType = float> class WriterBase {
public:
  using value_type = ValueType;

  WriterBase(const std::filesystem::path &path) : path(path), dtype(DataType::from<ValueType>()) {
    dtype.set_byte_order_native();
    if (dtype != DataType::Float32LE && dtype != DataType::Float32BE && dtype != DataType::Float64LE &&
        dtype != DataType::Float64BE)
      throw Exception("only supported datatype for tracks file are "
                      "Float32LE, Float32BE, Float64LE & Float64BE");
    App::check_overwrite(path);
  }

  ~WriterBase() noexcept {
    if (open_success) {
      try {
        File::OFStream out(path, std::ios_base::in | std::ios_base::out | std::ios_base::binary);
        update_counts(out);
      } catch (Exception &e) {
        e.display();
      }
    }
  }

  void create(File::OFStream &out, const Properties &properties, std::string_view type) {
    out << "mrtrix " + type + "\nEND\n";

    for (const auto &i : properties) {
      if ((i.first != "count") && (i.first != "total_count")) {
        for (const auto &line : split_lines(i.second))
          out << i.first << ": " << line << "\n";
      }
    }

    for (const auto &i : properties.comments)
      out << "comment: " << i << "\n";

    for (size_t n = 0; n < properties.seeds.num_seeds(); ++n)
      out << "roi: seed " << properties.seeds[n]->get_name() << "\n";
    for (size_t n = 0; n < properties.include.size(); ++n)
      out << "roi: include " << properties.include[n].parameters() << "\n";
    for (size_t n = 0; n < properties.exclude.size(); ++n)
      out << "roi: exclude " << properties.exclude[n].parameters() << "\n";
    for (size_t n = 0; n < properties.mask.size(); ++n)
      out << "roi: mask " << properties.mask[n].parameters() << "\n";

    for (const auto &it : properties.prior_rois)
      out << "prior_roi: " << it.first << " " << it.second << "\n";

    out << "datatype: " << dtype.specifier() << "\n";
    int64_t data_offset = static_cast<int64_t>(out.tellp()) + 65;
    data_offset += (4 - (data_offset % 4)) % 4;
    out << "file: . " << data_offset << "\n";
    out << "count: ";
    count_offset = out.tellp();
    out << "0\nEND\n";
    out.seekp(0);
    out << "mrtrix " + type + "    ";
    out.seekp(data_offset);
  }

  void skip() { ++total_count; }

  uint64_t count{0}, total_count{0};

protected:
  std::filesystem::path path;
  DataType dtype;
  int64_t count_offset{0};
  bool open_success{false};

  void verify_stream(const File::OFStream &out) {
    if (!out.good())
      throw Exception("error writing file \"" + path.string() + "\": " + MR::C_strerror(errno));
  }

  void update_counts(File::OFStream &out) {
    out.seekp(count_offset);
    out << count << "\ntotal_count: " << total_count << "\nEND\n";
    verify_stream(out);
  }
};

//! \endcond

} // namespace MR::DWI::Tractography
