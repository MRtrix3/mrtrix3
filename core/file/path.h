/* Copyright (c) 2008-2023 the MRtrix3 contributors.
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

#ifndef __file_path_h__
#define __file_path_h__

#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <string>
#include <cstdlib>
#include <cstring>
#include <cerrno>
#include <unistd.h>
#include <algorithm>

#include "exception.h"
#include "mrtrix.h"
#include "types.h"

#define HOME_ENV "HOME"

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
#define PATH_SEPARATORS "/\\"
#else
#define PATH_SEPARATORS "/"
#endif


namespace MR
{
  namespace Path
  {

    inline std::string basename (const std::string& name)
    {
      size_t i = name.find_last_of (PATH_SEPARATORS);
      return (i == std::string::npos ? name : name.substr (i+1));
    }


    inline std::string dirname (const std::string& name)
    {
      size_t i = name.find_last_of (PATH_SEPARATORS);
      return (i == std::string::npos ? std::string ("") : (i ? name.substr (0,i) : std::string(1, PATH_SEPARATORS[0])));
    }


    inline std::string join (const std::string& first, const std::string& second)
    {
      if (first.empty())
        return second;
      if (first[first.size()-1] != PATH_SEPARATORS[0]
#ifdef MRTRIX_WINDOWS
          && first[first.size()-1] != PATH_SEPARATORS[1]
#endif
          )
        return first + PATH_SEPARATORS[0] + second;
      return first + second;
    }


    inline bool exists (const std::string& path)
    {
      struct stat buf;
#ifdef MRTRIX_WINDOWS
      const std::string stripped (strip (path, PATH_SEPARATORS, false, true));
      if (!stat (stripped.c_str(), &buf))
#else
      if (!stat (path.c_str(), &buf))
#endif
        return true;
      if (errno == ENOENT) return false;
      throw Exception (strerror (errno));
      return false;
    }


    inline bool is_dir (const std::string& path)
    {
      struct stat buf;
#ifdef MRTRIX_WINDOWS
      const std::string stripped (strip (path, PATH_SEPARATORS, false, true));
      if (!stat (stripped.c_str(), &buf))
#else
      if (!stat (path.c_str(), &buf))
#endif
        return S_ISDIR (buf.st_mode);
      if (errno == ENOENT) return false;
      throw Exception (strerror (errno));
      return false;
    }


    inline bool is_file (const std::string& path)
    {
      struct stat buf;
      if (!stat (path.c_str(), &buf)) return S_ISREG (buf.st_mode);
      if (errno == ENOENT) return false;
      throw Exception (strerror (errno));
      return false;
    }


    inline bool has_suffix (const std::string& name, const std::string& suffix)
    {
      return name.size() >= suffix.size() &&
        name.compare(name.size() - suffix.size(), suffix.size(), suffix) == 0;
    }

    inline bool has_suffix (const std::string &name, const std::initializer_list<const std::string> &suffix_list)
    {
      return std::any_of (suffix_list.begin(),
                          suffix_list.end(),
                          [&] (const std::string& suffix) { return has_suffix (name, suffix); });
    }

    inline bool has_suffix (const std::string &name, const vector<std::string> &suffix_list)
    {
      return std::any_of (suffix_list.begin(),
                          suffix_list.end(),
                          [&] (const std::string& suffix) { return has_suffix (name, suffix); });
    }

    inline bool is_mrtrix_image (const std::string& name)
    {
      return strcmp(name.c_str(), std::string("-").c_str()) == 0 ||
        Path::has_suffix (name, {".mif", ".mih", ".mif.gz"});
    }

    inline std::string cwd ()
    {
      std::string path;
      size_t buf_size = 32;
      while (true) {
        path.reserve (buf_size);
        if (getcwd (&path[0], buf_size))
          break;
        if (errno != ERANGE)
          throw Exception ("failed to get current working directory!");
        buf_size *= 2;
      }
      return path;
    }

    inline std::string home ()
    {
      const char* home = getenv (HOME_ENV);
      if (!home)
        throw Exception (HOME_ENV " environment variable is not set!");
      return home;
    }

    class Dir { NOMEMALIGN
      public:
        Dir (const std::string& name) :
          p (opendir (name.size() ? name.c_str() : ".")) {
          if (!p)
            throw Exception ("error opening folder " + name + ": " + strerror (errno));
        }
        ~Dir () {
          if (p) closedir (p);
        }

        std::string read_name () {
          std::string ret;
          struct dirent* entry = readdir (p);
          if (entry) {
            ret = entry->d_name;
            if (ret == "." || ret == "..")
              ret = read_name();
          }
          return ret;
        }
        void rewind () {
          rewinddir (p);
        }
        void close () {
          if (p) closedir (p);
          p = NULL;
        }

      protected:
        DIR* p;
    };



    inline char delimiter (const std::string& filename)
    {
      if (Path::has_suffix (filename, ".tsv"))
        return '\t';
      else if (Path::has_suffix (filename, ".csv"))
        return ',';
      else
        return ' ';
    }


  }
}

#endif

