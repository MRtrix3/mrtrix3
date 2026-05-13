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

#include <array>
#include <cstddef>
#include <fcntl.h>
#include <unistd.h>
#include <zlib.h>

#if defined(MRTRIX_MACOSX) || defined(MRTRIX_FREEBSD)
#include <sys/mount.h>
#include <sys/param.h>
#elif !defined(MRTRIX_WINDOWS)
#include <sys/vfs.h>
#endif

#ifdef MRTRIX_WINDOWS
#include <windows.h>
#else
#include <sys/mman.h>
#endif

#include "app.h"
#include "file/config.h"
#include "file/mmap.h"
#include "file/ofstream.h"
#include "file/path.h"

#include "debug.h"

namespace MR::File {

MMap::MMap(const Entry &entry, bool readwrite, bool preload, std::optional<int64_t> mapped_size)
    : Entry(entry), addr(nullptr), first(nullptr), msize(0), readwrite(readwrite) {

  DEBUG("memory-mapping file \"" + Entry::path.string() + "\"...");

  try {
    auto last_write = std::filesystem::last_write_time(Entry::path);
    auto sctp = std::chrono::time_point_cast<std::chrono::system_clock::duration>(
        last_write - std::filesystem::file_time_type::clock::now() + std::chrono::system_clock::now());
    mtime = std::chrono::system_clock::to_time_t(sctp);
    const int64_t file_size = std::filesystem::file_size(Entry::path);
    if (!mapped_size.has_value()) {
      msize = file_size - start;
    } else {
      msize = mapped_size.value();
      if (start + msize > file_size)
        throw Exception("file \"" + Entry::path.string() + "\" is smaller than expected");
    }
  } catch (const std::exception &e) {
    throw Exception("cannot stat file \"" + Entry::path.string() + "\": " + e.what());
  }

  bool delayed_writeback = false;
  if (readwrite) {

#ifdef MRTRIX_WINDOWS
    const unsigned int length = 255;
    std::array<char, length> root_path;
    // Returns non-zero on success
    if (GetVolumePathName(Entry::path.string().c_str(), root_path.data(), length) != 0) {

      const unsigned int code = GetDriveType(root_path.data());
      switch (code) {
      case 0: // DRIVE_UNKNOWN
        DEBUG("cannot get filesystem information on file \"" + Entry::path.string() + "\": " + MR::C_strerror(errno));
        DEBUG("  defaulting to delayed write-back");
        delayed_writeback = true;
        break;
      case 1: // DRIVE_NO_ROOT_DIR:
        DEBUG("erroneous root path derived for file \"" + Entry::path.string() + "\": " + MR::C_strerror(errno));
        DEBUG("  defaulting to delayed write-back");
        delayed_writeback = true;
        break;
      case 2: // DRIVE_REMOVABLE
        DEBUG("Drive for file \"" + Entry::path.string() + "\" detected as removable; using memory-mapping");
        break;
      case 3: // DRIVE_FIXED
        DEBUG("Drive for file \"" + Entry::path.string() + "\" detected as fixed; using memory-mapping");
        break;
      case 4: // DRIVE_REMOTE
        DEBUG("Drive for file \"" + Entry::path.string() + "\" detected as network - using delayed write-back");
        delayed_writeback = true;
        break;
      case 5: // DRIVE_CDROM
        DEBUG("Drive for file \"" + Entry::path.string() + "\" detected as CD-ROM - using delayed write-back");
        delayed_writeback = true;
        break;
      case 6: // DRIVE_RAMDISK
        DEBUG("Drive for file \"" + Entry::path.string() + "\" detected as RAM - using memory-mapping");
        break;
      }

    } else {
      DEBUG("unable to query root drive path for file \"" + Entry::path.string() + "\"; using delayed write-back");
      delayed_writeback = true;
    }
#else
    struct statfs fsbuf;
    if (statfs(Entry::path.string().c_str(), &fsbuf)) {
      DEBUG("cannot get filesystem information on file \"" + Entry::path.string() + "\": " + MR::C_strerror(errno));
      DEBUG("  defaulting to delayed write-back");
      delayed_writeback = true;
    }

    if (fsbuf.f_type == 0xff534d42 /* CIFS */ || fsbuf.f_type == 0x6969 /* NFS */ ||       //
        fsbuf.f_type == 0x65735546 /* FUSE */ || fsbuf.f_type == 0x517b /* SMB */ ||       //
        fsbuf.f_type == 0x47504653 /* GPFS */ || fsbuf.f_type == 0xbd00bd0 /* LUSTRE */ || //
        fsbuf.f_type == 0x1021997 /* 9P (WSL) */                                           //
#ifdef MRTRIX_MACOSX
        || fsbuf.f_type == 0x0017 /* OSXFUSE */                                            //
#endif
    ) {                                                                                    //
      DEBUG("\"" + Entry::path.string() + "\" appears to reside on a networked filesystem - using delayed write-back");
      delayed_writeback = true;
    }
#endif

    if (delayed_writeback) {
      try {
        first = new std::byte[msize];
        if (!first)
          throw 1;
      } catch (...) {
        throw Exception("error allocating memory to hold mmap buffer contents");
      }

      if (preload) {
        CONSOLE("preloading contents of mapped file \"" + Entry::path.string() + "\"...");
        std::ifstream in(Entry::path, std::ios::in | std::ios::binary);
        if (!in)
          throw Exception("failed to open file \"" + Entry::path.string() + "\": " + MR::C_strerror(errno));
        in.seekg(start, in.beg);
        in.read(reinterpret_cast<char *>(first), msize);
        if (!in.good())
          throw Exception("error preloading contents of file \"" + Entry::path.string() + "\": " + MR::C_strerror(errno));
      } else
        memset(first, 0, msize);
      DEBUG("file \"" + Entry::path.string() + "\" held in RAM at " + str(reinterpret_cast<void *>(first)) + "," + //
            " size " + str(msize));                                                                                //

      return;
    }
  }

  // use regular memory-mapping:
  fd = open(Entry::path.string().c_str(), (readwrite ? O_RDWR : O_RDONLY), 0666);
  if (fd < 0)
    throw Exception("error opening file \"" + Entry::path.string() + "\": " + MR::C_strerror(errno));

  try {
#ifdef MRTRIX_WINDOWS
    HANDLE handle = CreateFileMapping(
        (HANDLE)_get_osfhandle(fd), nullptr, (readwrite ? PAGE_READWRITE : PAGE_READONLY), 0, start + msize, nullptr);
    if (!handle)
      throw 0;
    addr = static_cast<std::byte *>(
        MapViewOfFile(handle, (readwrite ? FILE_MAP_ALL_ACCESS : FILE_MAP_READ), 0, 0, start + msize));
    if (!addr)
      throw 0;
    CloseHandle(handle);
#else
    addr = static_cast<std::byte *>(
        mmap(nullptr, start + msize, (readwrite ? PROT_WRITE | PROT_READ : PROT_READ), MAP_SHARED, fd, 0));
    if (addr == MAP_FAILED)
      throw 0;
#endif
  } catch (...) {
    close(fd);
    addr = nullptr;
    throw Exception("memory-mapping failed for file \"" + Entry::path.string() + "\": " + MR::C_strerror(errno));
  }
  first = addr + start;
  DEBUG("file \"" + Entry::path.string() + "\" mapped at " + str(reinterpret_cast<void *>(addr)) + "," + //
        " size " + str(msize) + " (read-" + (readwrite ? "write" : "only") + ")");                       //
}

MMap::~MMap() {
  if (!first)
    return;
  if (addr) {
    DEBUG("unmapping file \"" + Entry::path.string() + "\"");
#ifdef MRTRIX_WINDOWS
    if (!UnmapViewOfFile(static_cast<LPVOID>(addr)))
#else
    if (munmap(addr, msize))
#endif
      WARN("error unmapping file \"" + Entry::path.string() + "\": " + MR::C_strerror(errno));
    close(fd);
  } else {
    if (readwrite) {
      INFO("writing back contents of mapped file \"" + Entry::path.string() + "\"...");
      try {
        File::OFStream out(Entry::path, std::ios::in | std::ios::out | std::ios::binary);
        out.seekp(start, out.beg);
        out.write(reinterpret_cast<const char *>(first), msize);
        if (!out.good())
          throw 1;
      } catch (...) {
        FAIL("error writing back contents of file \"" + Entry::path.string() + "\": " + MR::C_strerror(errno));
        App::exit_error_code = 1;
      }
    }
    delete[] first;
  }
}

bool MMap::changed() const {
  assert(fd >= 0);
  try {
    const int64_t file_size = std::filesystem::file_size(Entry::path);
    if (static_cast<int64_t>(msize) != file_size)
      return true;
    auto last_write = std::filesystem::last_write_time(Entry::path);
    auto sctp = std::chrono::time_point_cast<std::chrono::system_clock::duration>(
        last_write - std::filesystem::file_time_type::clock::now() + std::chrono::system_clock::now());
    const time_t current_mtime = std::chrono::system_clock::to_time_t(sctp);
    if (mtime != current_mtime)
      return true;
    return false;
  } catch (...) {
    return false;
  }
}

} // namespace MR::File
