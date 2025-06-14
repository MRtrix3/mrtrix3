/* Copyright (c) 2008-2025 the MRtrix3 contributors.
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
#include <unistd.h>

#include "app.h"
#include "exception.h"
#include "file/config.h"
#include "file/path.h"

namespace MR::File {

namespace {
inline char random_char() {
  const char c = rand() % 62;
  if (c < 10)
    return c + 48;
  if (c < 36)
    return c + 55;
  return c + 61;
}

// CONF option: TmpFileDir
// CONF default: `/tmp` (on Unix), `.` (on Windows)
// CONF The prefix for temporary files (as used in pipelines). By default,
// CONF these files get written to the current folder on Windows machines,
// CONF which may cause performance issues, particularly when operating
// CONF over distributed file systems. On Unix machines, the default is
// CONF /tmp/, which is typically a RAM file system and should therefore
// CONF be fast; but may cause issues on machines with little RAM
// CONF capacity or where write-access to this location is not permitted.
// CONF
// CONF Note that this location can also be manipulated using the
// CONF :envvar:`MRTRIX_TMPFILE_DIR` environment variable, without editing the
// CONF config file. Note also that this setting does not influence the
// CONF location in which Python scripts construct their scratch
// CONF directories; that is determined based on config file option
// CONF ScriptScratchDir.

// ENVVAR name: MRTRIX_TMPFILE_DIR
// ENVVAR This has the same effect as the :option:`TmpFileDir`
// ENVVAR configuration file entry, and can be used to set the location of
// ENVVAR temporary files (as used in Unix pipes) for a single session,
// ENVVAR within a single script, or for a single command without
// ENVVAR modifying the configuration  file.
const std::string __get_tmpfile_dir() {
  const char *from_env_mrtrix = getenv("MRTRIX_TMPFILE_DIR");
  if (from_env_mrtrix != nullptr)
    return from_env_mrtrix;

  const char *default_tmpdir =
#ifdef MRTRIX_WINDOWS
      "."
#else
      "/tmp"
#endif
      ;

  const char *from_env_general = getenv("TMPDIR");
  if (from_env_general != nullptr)
    default_tmpdir = from_env_general;

  return File::Config::get("TmpFileDir", default_tmpdir);
}

const std::string &tmpfile_dir() {
  static const std::string __tmpfile_dir = __get_tmpfile_dir();
  return __tmpfile_dir;
}

// CONF option: TmpFilePrefix
// CONF default: `mrtrix-tmp-`
// CONF The prefix to use for the basename of temporary files. This will
// CONF be used to generate a unique filename for the temporary file, by
// CONF adding random characters to this prefix, followed by a suitable
// CONF suffix (depending on file type). Note that this prefix can also be
// CONF manipulated using the `MRTRIX_TMPFILE_PREFIX` environment
// CONF variable, without editing the config file.

// ENVVAR name: MRTRIX_TMPFILE_PREFIX
// ENVVAR This has the same effect as the :option:`TmpFilePrefix`
// ENVVAR configuration file entry, and can be used to set the prefix for
// ENVVAR the name  of temporary files (as used in Unix pipes) for a
// ENVVAR single session, within a single script, or for a single command
// ENVVAR without modifying the configuration file.
const std::string __get_tmpfile_prefix() {
  const char *from_env = getenv("MRTRIX_TMPFILE_PREFIX");
  if (from_env != nullptr)
    return from_env;
  return File::Config::get("TmpFilePrefix", "mrtrix-tmp-");
}

const std::string &tmpfile_prefix() {
  static const std::string __tmpfile_prefix = __get_tmpfile_prefix();
  return __tmpfile_prefix;
}

} // namespace

void remove(const std::string &file) {
  if (std::remove(file.c_str()) != 0)
    throw Exception("error deleting file \"" + file + "\": " + strerror(errno));
}

void create(const std::string &filename, int64_t size) {
  DEBUG(std::string("creating ") + (size ? "" : "empty ") + "file \"" + filename + "\"" +
        (size == 0 ? "" : (" with size " + str(size))));

  int fid(0);
  while ((fid = open(filename.c_str(),
                     O_CREAT | O_RDWR | O_EXCL,
                     S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH)) < 0) {
    if (errno == EEXIST) {
      App::check_overwrite(filename);
      INFO("file \"" + filename + "\" already exists - removing");
      remove(filename);
    } else
      throw Exception("error creating output file \"" + filename + "\": " + std::strerror(errno));
  }
  if (fid < 0) {
    std::string mesg("error creating file \"" + filename + "\": " + strerror(errno));
    if (errno == EEXIST)
      mesg += " (use -force option to force overwrite)";
    throw Exception(mesg);
  }

  if (size != 0)
    size = ftruncate(fid, size);
  close(fid);

  if (size != 0)
    throw Exception("cannot resize file \"" + filename + "\": " + strerror(errno));
}

void resize(const std::string &filename, int64_t size) {
  DEBUG("resizing file \"" + filename + "\" to " + str(size));

  const int fd = open(filename.c_str(), O_RDWR, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);
  if (fd < 0)
    throw Exception("error opening file \"" + filename + "\" for resizing: " + strerror(errno));
  const int status = ftruncate(fd, size);
  close(fd);
  if (status != 0)
    throw Exception("cannot resize file \"" + filename + "\": " + strerror(errno));
}

bool is_tempfile(const std::filesystem::path &name, const char *suffix) {
  if (name.filename().string().compare(0, tmpfile_prefix().size(), tmpfile_prefix()) != 0)
    return false;
  if (suffix != nullptr)
    if (!Path::has_suffix(name, suffix))
      return false;
  return true;
}

std::string create_tempfile(int64_t size, const char *suffix) {
  DEBUG("creating temporary file of size " + str(size));

  std::string filename(Path::join(tmpfile_dir(), tmpfile_prefix()) + "XXXXXX.");
  const int rand_index = filename.size() - 7;
  if (suffix != nullptr)
    filename += suffix;

  int fid(0);
  do {
    for (int n = 0; n < 6; n++)
      filename[rand_index + n] = random_char();
    fid = open(filename.c_str(), O_CREAT | O_RDWR | O_EXCL, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);
  } while (fid < 0 && errno == EEXIST);

  if (fid < 0)
    throw Exception(std::string("error creating temporary file in directory \"" + tmpfile_dir() + "\": ") +
                    strerror(errno));

  const int status = size == 0 ? 0 : ftruncate(fid, size);
  close(fid);
  if (status)
    throw Exception("cannot resize file \"" + filename + "\": " + strerror(errno));

  return filename;
}

void mkdir(const std::string &folder) {
  if (::mkdir(folder.c_str()
#ifndef MRTRIX_WINDOWS
                  ,
              0777
#endif
              ) != 0)
    throw Exception("error creating folder \"" + folder + "\": " + strerror(errno));
}

void rmdir(const std::string &folder, bool recursive) {
  if (recursive) {
    Path::Dir dir(folder);
    std::string entry;
    while (!(entry = dir.read_name()).empty()) {
      std::string path = Path::join(folder, entry);
      if (Path::is_dir(path))
        rmdir(path, true);
      else
        remove(path);
    }
  }
  DEBUG("deleting folder \"" + folder + "\"...");
  if (::rmdir(folder.c_str()))
    throw Exception("error deleting folder \"" + folder + "\": " + strerror(errno));
}

} // namespace MR::File
