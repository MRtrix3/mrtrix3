/*******************************************************************************
    Copyright (C) 2014 Brain Research Institute, Melbourne, Australia
    
    Permission is hereby granted under the Patent Licence Agreement between
    the BRI and Siemens AG from July 3rd, 2012, to Siemens AG obtaining a
    copy of this software and associated documentation files (the
    "Software"), to deal in the Software without restriction, including
    without limitation the rights to possess, use, develop, manufacture,
    import, offer for sale, market, sell, lease or otherwise distribute
    Products, and to permit persons to whom the Software is furnished to do
    so, subject to the following conditions:
    
    The above copyright notice and this permission notice shall be included
    in all copies or substantial portions of the Software.
    
    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
    OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
    MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
    IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
    CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
    TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
    SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

*******************************************************************************/


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

#ifdef MRTRIX_WINDOWS
#define PATH_SEPARATOR "\\/"
#define HOME_ENV "USERPROFILE"
#else
#define PATH_SEPARATOR "/"
#define HOME_ENV "HOME"
#endif


namespace MR
{
  namespace Path
  {

    inline std::string basename (const std::string& name)
    {
      size_t i = name.find_last_of (PATH_SEPARATOR);
      return (i == std::string::npos ? name : name.substr (i+1));
    }


    inline std::string dirname (const std::string& name)
    {
      size_t i = name.find_last_of (PATH_SEPARATOR);
      return (i == std::string::npos ? std::string ("") : (i ? name.substr (0,i) : std::string (PATH_SEPARATOR)));
    }


    inline std::string join (const std::string& first, const std::string& second)
    {
      if (first.empty()) 
        return second;
      if (first[first.size()-1] != PATH_SEPARATOR[0]
#ifdef MRTRIX_WINDOWS
          && first[first.size()-1] != PATH_SEPARATOR[1]
#endif
          ) 
        return first + PATH_SEPARATOR[0] + second;
      return first + second;
    }


    inline bool exists (const std::string& path)
    {
      struct stat buf;
      if (!stat (path.c_str(), &buf)) return true;
      if (errno == ENOENT) return false;
      throw Exception (strerror (errno));
      return false;
    }


    inline bool is_dir (const std::string& path)
    {
      struct stat buf;
      if (!stat (path.c_str(), &buf)) return S_ISDIR (buf.st_mode);
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
      return (name.size() < suffix.size() ?
              false :
              name.substr (name.size()-suffix.size()) == suffix);
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

    class Dir
    {
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

  }
}

#endif

