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

#include "file/utils.h"

#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

#include "app.h"
#include "exception.h"
#include <fmt/std.h>

namespace MR::File {

void remove(const std::filesystem::path &path) {
  if (std::remove(path.string().c_str()) != 0)
    throw Exception(fmt::format("error deleting file \"{}\": {}", path, strerror(errno)));
}

void create(const std::filesystem::path &path, int64_t size) {
  DEBUG(fmt::format("{}{}file \"{}\"{}",
                    "creating ",
                    (size != 0 ? "" : "empty "),
                    path,
                    (size == 0 ? "" : fmt::format(" with size {}", size))));

  int fid(0);
  while ((fid = open(path.string().c_str(),                                     //
                     O_CREAT | O_RDWR | O_EXCL,                                 //
                     S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH) //
          ) < 0) {                                                              //
    if (errno == EEXIST) {
      App::check_overwrite(path);
      INFO(fmt::format("file \"{}\" already exists - removing", path));
      MR::File::remove(path);
    } else
      throw Exception(fmt::format("error creating output file \"{}\": {}", path, strerror(errno)));
  }

  if (size != 0) {
    const int status = ftruncate(fid, size);
    close(fid);
    if (status != 0)
      throw Exception(fmt::format("cannot resize file \"{}\": {}", path, strerror(errno)));
  } else {
    close(fid);
  }
}

void resize(const std::filesystem::path &path, int64_t size) {
  DEBUG(fmt::format("resizing file \"{}\" to {}", path, size));

  const int fd = open(path.string().c_str(), O_RDWR, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);
  if (fd < 0)
    throw Exception(fmt::format("error opening file \"{}\" for resizing: {}", path, strerror(errno)));
  const int status = ftruncate(fd, size);
  close(fd);
  if (status != 0)
    throw Exception(fmt::format("cannot resize file \"{}\": {}", path, strerror(errno)));
}

void mkdir(const std::filesystem::path &folder) {
  std::error_code ec;
  std::filesystem::create_directory(folder, ec);
  if (ec)
    throw Exception(fmt::format("error creating folder \"{}\": {}", folder, ec.message()));
}

void rmdir(const std::filesystem::path &folder, bool recursive) {
  if (recursive) {
    for (const auto &entry : std::filesystem::directory_iterator(folder)) {
      if (std::filesystem::is_directory(entry.path()))
        rmdir(entry.path(), true);
      else
        MR::File::remove(entry.path());
    }
  }
  DEBUG(fmt::format("deleting folder \"{}\"...", folder));
  std::error_code ec;
  std::filesystem::remove(folder, ec);
  if (ec)
    throw Exception(fmt::format("error deleting folder \"{}\": {}", folder, ec.message()));
}

} // namespace MR::File
