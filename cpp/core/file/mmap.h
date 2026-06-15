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

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <iostream>
#include <optional>

#include "file/entry.h"

namespace MR::File {

class MMap : protected Entry {
public:
  //! create a new memory-mapping to file in \a entry
  /*! map file in \a entry at the offset in \a entry. By default, the
   * file will be mapped read-only. If \a readwrite is set to true,
   * the file will be accessed with read-write permissions, but the
   * mechanism used depends on whether the file is detected as residing
   * on a local or a networked filesystem, and whether the filesystem is
   * mounted with synchronous IO. If the filesystem is \e local and
   * \e asynchronous, the file is memory-mapped as-is with read-write
   * permissions. Otherwise, a write-back RAM buffer is allocated to
   * store the contents of the file, and written back when the
   * constructor is invoked.
   *
   * By default, if the file is mapped using the delayed write-back
   * mechanism, its contents will be preloaded into the RAM buffer. If
   * the file has just been created, \a preload should be set to \c false to
   * prevent this, in which case the contents will set to zero.
   *
   * By default, the whole file is mapped. If \a mapped_size is set,
   * then only the region of that size starting from the byte offset
   * specified in \a entry will be mapped.
   */
  MMap(const Entry &entry,
       bool readwrite = false,
       bool preload = true,
       std::optional<int64_t> mapped_size = std::nullopt);
  ~MMap();

  [[nodiscard]] std::filesystem::path path() const { return Entry::path; }
  [[nodiscard]] int64_t size() const { return msize; }
  std::byte *const address() { return first; }
  [[nodiscard]] const std::byte *const address() const { return first; }

  [[nodiscard]] bool is_read_write() const { return readwrite; }
  [[nodiscard]] bool changed() const;

  friend std::ostream &operator<<(std::ostream &stream, const MMap &m) {
    stream << "File::MMap { " << m.path().string() << " [" << m.fd << "], size: " << m.size() << ", mapped "
           << (m.readwrite ? "RW" : "RO") << " at " << reinterpret_cast<const void *>(m.address()) << ", offset "
           << m.start << " }";
    return stream;
  }

protected:
  int fd{0};
  std::byte *addr{nullptr};  /**< The address in memory where the file has been mapped. */
  std::byte *first{nullptr}; /**< The address in memory to the start of the region of interest. */
  int64_t msize{0};          /**< The size of the mapped portion of the file. */
  time_t mtime{0};           /**< The modification time of the file at the last check. */
  bool readwrite{false};

  void map();

private:
  MMap(const MMap &mmap) : Entry(mmap) { assert(0); }
};

} // namespace MR::File
