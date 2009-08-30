/*
   Copyright 2008 Brain Research Institute, Melbourne, Australia

   Written by J-Donald Tournier, 27/06/08.

   This file is part of MRtrix.

   MRtrix is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   MRtrix is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with MRtrix.  If not, see <http://www.gnu.org/licenses/>.

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

#include "types.h"
#include "exception.h"

#ifdef WINDOWS
#define PATH_SEPARATOR "\\/"
#else 
#define PATH_SEPARATOR "/"
#endif


namespace MR {
  namespace Path {

    inline std::string basename (const std::string& name) {
      size_t i = name.find_last_of (PATH_SEPARATOR);
      return (i == std::string::npos ? name : name.substr (i+1));
    }


    inline std::string dirname (const std::string& name) {
      size_t i = name.find_last_of (PATH_SEPARATOR);
      return (i == std::string::npos ? std::string("") : ( i ? name.substr (0,i) : std::string(PATH_SEPARATOR) ));
    }


    inline std::string join (const std::string& first, const std::string& second) 
    { return (first.empty() ? second : first + std::string(PATH_SEPARATOR)[0] + second); }


    inline bool exists (const std::string& path) 
    { 
      struct stat64 buf;
      if (!stat64 (path.c_str(), &buf)) return (true);
      if (errno == ENOENT) return (false);
      throw Exception (strerror (errno)); 
      return (false);
    }


    inline bool is_dir (const std::string& path) 
    { 
      struct stat64 buf;
      if (!stat64 (path.c_str(), &buf)) return (S_ISDIR(buf.st_mode));
      if (errno == ENOENT) return (false);
      throw Exception (strerror (errno)); 
      return (false);
    }


    inline bool is_file (const std::string& path) 
    { 
      struct stat64 buf;
      if (!stat64 (path.c_str(), &buf)) return (S_ISREG(buf.st_mode));
      if (errno == ENOENT) return (false);
      throw Exception (strerror (errno)); 
      return (false);
    }

    inline bool has_suffix (const std::string& name, const std::string& suffix) 
    { return (name.size() < suffix.size() ? false : name.substr (name.size()-suffix.size()) == suffix); }


    inline std::string cwd (size_t buf_size = 32) 
    { 
      char buf [buf_size];
      if (getcwd (buf, buf_size)) return (buf);
      if (errno != ERANGE) throw Exception ("failed to get current working directory!");
      return (cwd (buf_size * 2));
    }

    inline std::string home () 
    { 
      const char* home = getenv ("HOME");
      if (!home) throw Exception ("HOME environment variable is not set!");
      return (home); 
    }

    class Dir 
    {
      public:
        Dir (const std::string& name) : p (opendir (name.c_str())) { if (!p) { throw Exception ("error opening folder " + name + ": " + strerror (errno)); } }
        ~Dir () { if (p) closedir (p); }

        std::string read_name () { 
          std::string ret; 
          struct dirent* entry = readdir(p); 
          if (entry) {
            ret = entry->d_name;
            if (ret == "." || ret == "..") ret = read_name();
          }
          return (ret); 
        }
        void rewind () { rewinddir (p); }
        void close () { if (p) closedir (p); p = NULL; }

      protected: 
        DIR* p;
    };

  }
}

#endif

