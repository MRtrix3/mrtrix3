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
#include <random>
#include <string>
#include <unistd.h>

#include "app.h"
#include "env.h"
#include "exception.h"
#include "file/config.h"
#include "file/path.h"

namespace MR::File {

namespace {
inline char random_char() {
  thread_local std::mt19937 rng{std::random_device{}()};
  thread_local std::uniform_int_distribution<int> dist{0, 61};
  const char c = dist(rng);
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
std::string _get_tmpfile_dir() {
  const std::optional<std::string> from_env_mrtrix = MR::get_env("MRTRIX_TMPFILE_DIR");
  if (from_env_mrtrix.has_value())
    return *from_env_mrtrix;

  const std::string default_tmpdir =
#ifdef MRTRIX_WINDOWS
      "."
#else
      "/tmp"
#endif
      ;

  return File::Config::get("TmpFileDir", MR::get_env("TMPDIR", default_tmpdir));
}

std::string tmpfile_dir() {
  static const std::string _tmpfile_dir = _get_tmpfile_dir();
  return _tmpfile_dir;
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
std::string _get_tmpfile_prefix() {
  return MR::get_env("MRTRIX_TMPFILE_PREFIX", File::Config::get("TmpFilePrefix", "mrtrix-tmp-"));
}

std::string tmpfile_prefix() {
  static const std::string _tmpfile_prefix = _get_tmpfile_prefix();
  return _tmpfile_prefix;
}

} // namespace

void remove(std::string_view filename) {
  const std::string temp(filename);
  if (std::remove(temp.c_str()) != 0)
    throw Exception("error deleting file \"" + temp + "\": " + MR::C_strerror(errno));
}

void create(std::string_view filename, int64_t size) {
  const std::string temp(filename);
  DEBUG(std::string("creating ") + (size ? "" : "empty ") + "file \"" + temp + "\"" +
        (size == 0 ? "" : (" with size " + str(size))));

  int fid(0);
  while ((fid = open(temp.c_str(),                                              //
                     O_CREAT | O_RDWR | O_EXCL,                                 //
                     S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH) //
          ) < 0) {                                                              //
    if (errno == EEXIST) {
      App::check_overwrite(filename);
      INFO("file \"" + temp + "\" already exists - removing");
      remove(filename);
    } else
      throw Exception("error creating output file \"" + temp + "\": " + MR::C_strerror(errno));
  }
  if (fid < 0) {
    std::string mesg("error creating file \"" + temp + "\": " + MR::C_strerror(errno));
    if (errno == EEXIST)
      mesg += " (use -force option to force overwrite)";
    throw Exception(mesg);
  }

  if (size != 0)
    size = ftruncate(fid, size);
  close(fid);

  if (size != 0)
    throw Exception("cannot resize file \"" + filename + "\": " + MR::C_strerror(errno));
}

void resize(std::string_view filename, int64_t size) {
  const std::string temp(filename);
  DEBUG("resizing file \"" + temp + "\" to " + str(size));

  const int fd = open(temp.c_str(), O_RDWR, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);
  if (fd < 0)
    throw Exception("error opening file \"" + temp + "\" for resizing: " + MR::C_strerror(errno));
  const int status = ftruncate(fd, size);
  close(fd);
  if (status != 0)
    throw Exception("cannot resize file \"" + temp + "\": " + MR::C_strerror(errno));
}

bool is_tempfile(std::string_view name, std::string_view suffix) {
  if (Path::basename(name).compare(0, tmpfile_prefix().size(), tmpfile_prefix()) != 0)
    return false;
  if (!suffix.empty() && !Path::has_suffix(name, suffix))
    return false;
  return true;
}

std::string create_tempfile(int64_t size, std::string_view suffix) {
  DEBUG("creating temporary file of size " + str(size));

  std::string filename(Path::join(tmpfile_dir(), tmpfile_prefix()) + "XXXXXX.");
  const int rand_index = filename.size() - 7;
  filename += suffix;

  int fid(0);
  do {
    for (int n = 0; n < 6; n++)
      filename[rand_index + n] = random_char();
    fid = open(filename.c_str(), O_CREAT | O_RDWR | O_EXCL, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);
  } while (fid < 0 && errno == EEXIST);

  if (fid < 0)
    throw Exception(std::string("error creating temporary file in directory \"" + tmpfile_dir() + "\": ") +
                    MR::C_strerror(errno));

  const int status = size == 0 ? 0 : ftruncate(fid, size);
  close(fid);
  if (status)
    throw Exception("cannot resize file \"" + filename + "\": " + MR::C_strerror(errno));

  return filename;
}

void mkdir(std::string_view folder) {
  const std::string temp(folder);
  if (::mkdir(temp.c_str()
#ifndef MRTRIX_WINDOWS
                  ,
              0777
#endif
              ) != 0)
    throw Exception("error creating folder \"" + temp + "\": " + MR::C_strerror(errno));
}

void rmdir(std::string_view folder, bool recursive) {
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
  const std::string temp(folder);
  DEBUG("deleting folder \"" + temp + "\"...");
  if (::rmdir(temp.c_str()) != 0)
    throw Exception("error deleting folder \"" + temp + "\": " + MR::C_strerror(errno));
}

} // namespace MR::File
