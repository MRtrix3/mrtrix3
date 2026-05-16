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

#include "file/temp.h"

#include <fcntl.h>
#include <random>
#include <string>
#include <sys/stat.h>
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
  const char c = static_cast<const char>(dist(rng));
  if (c < 10)
    return c + 48;
  if (c < 36)
    return c + 55;
  return c + 61;
}

// CONF option: TmpFileDir
// CONF default: System temporary directory (typically /tmp).
// CONF The prefix for temporary files (as used in pipelines). By default,
// CONF these files get written to the system temporary directory, which is
// CONF typically a RAM file system on Unix machines and should therefore
// CONF be fast; but may cause issues on machines with little RAM capacity
// CONF or where write-access to this location is not permitted.
// CONF On Windows MSYS2 this is likely /tmp relative to the MSYS2
// CONF installation directory (eg. C:\msys64\tmp\).
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
std::filesystem::path _get_tmpfile_dir() {
  const std::optional<std::filesystem::path> from_env_mrtrix = MR::get_env("MRTRIX_TMPFILE_DIR");
  if (from_env_mrtrix.has_value())
    return from_env_mrtrix.value();
  return {File::Config::get("TmpFileDir", MR::get_env("TMPDIR", std::filesystem::temp_directory_path().string()))};
}

const std::filesystem::path &tmpfile_dir() {
  static const std::filesystem::path _tmpfile_dir{_get_tmpfile_dir()};
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

bool is_tempfile(const std::filesystem::path &path, std::string_view suffix) {
  if (path.filename().string().compare(0, tmpfile_prefix().size(), tmpfile_prefix()) != 0)
    return false;
  if (!suffix.empty())
    if (!Path::has_suffix(path, suffix))
      return false;
  return true;
}

std::filesystem::path create_tempfile(int64_t size, std::string_view suffix) {
  DEBUG("creating temporary file of size " + str(size));

  int fid(0);
  std::filesystem::path filepath;
  std::string random_chars(6, '\0');
  do {
    for (int n = 0; n < 6; n++)
      random_chars[n] = random_char();
    filepath = (tmpfile_dir() / (tmpfile_prefix() + random_chars + suffix));
    fid = open(filepath.string().c_str(),
               O_CREAT | O_RDWR | O_EXCL,
               S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);
  } while (fid < 0 && errno == EEXIST);

  if (fid < 0)
    throw Exception(std::string("error creating temporary file in directory \"" + tmpfile_dir().string() + "\": ") +
                    MR::C_strerror(errno));

  const int status = size == 0 ? 0 : ftruncate(fid, size);
  close(fid);
  if (status)
    throw Exception("cannot resize file \"" + filepath.string() + "\": " + MR::C_strerror(errno));

  return filepath;
}

} // namespace MR::File
