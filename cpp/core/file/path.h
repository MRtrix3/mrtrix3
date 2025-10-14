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

#pragma once

#include <algorithm>
#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <dirent.h>
#include <string>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "exception.h"
#include "mrtrix.h"
#include "types.h"

/*! \def PATH_SEPARATORS
 *  \brief symbols used for separating directories in filesystem paths
 *
 *  The PATH_SEPARATORS macro contains all characters that may be used
 *  to delimit directory / file names in a filesystem path. On
 *  POSIX-compliant systems, this is simply the forward-slash character
 *  '/'; on Windows however, either forward-slashes or back-slashes
 *  can appear. Therefore any code that performs such direct
 *  manipulation of filesystem paths should both use this macro, and
 *  be written accounting for the possibility of this string containing
 *  either one or two characters depending on the target system. */
#ifdef MRTRIX_WINDOWS
// Preferentially use forward-slash when inserting via PATH_SEPARATORS[0]
#define PATH_SEPARATORS "/\\" // check_syntax off
#else
#define PATH_SEPARATORS "/" // check_syntax off
#endif

namespace MR::Path {

extern const std::string home_env;

std::string basename(const std::string &name);
std::string dirname(const std::string &name);
std::string join(const std::string &first, const std::string &second);
bool exists(const std::string &path);
bool is_dir(const std::string &path);
bool is_file(const std::string &path);
bool has_suffix(const std::string &name, const std::string &suffix);
bool has_suffix(const std::string &name, const std::initializer_list<const std::string> &suffix_list);
bool has_suffix(const std::string &name, const std::vector<std::string> &suffix_list);
bool is_mrtrix_image(const std::string &name);
char delimiter(const std::string &filename);
std::string cwd();
std::string home();

class Dir {
public:
  Dir(const std::string &name);
  ~Dir();

  std::string read_name();
  void rewind() { rewinddir(p); }
  void close();

protected:
  DIR *p;
};

} // namespace MR::Path
