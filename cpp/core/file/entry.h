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
#include <string>

#include "types.h"

namespace MR::File {

class Entry {
public:
  Entry(const std::filesystem::path &fpath, int64_t offset = 0) : path(fpath), start(offset) {}

  Entry(const Entry &) = default;
  Entry(Entry &&) noexcept = default;
  Entry &operator=(Entry &&E) noexcept {
    path = std::move(E.path);
    start = E.start;
    return *this;
  }

  std::filesystem::path path;
  int64_t start;
};

inline std::ostream &operator<<(std::ostream &stream, const Entry &e) {
  stream << "File::Entry { \"" << e.path.string() << "\", offset " << e.start << " }";
  return stream;
}
} // namespace MR::File
