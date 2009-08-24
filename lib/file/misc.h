/*
    Copyright 2008 Brain Research Institute, Melbourne, Australia

    Written by J-Donald Tournier, 23/07/09.

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

#ifndef __file_ops_h__
#define __file_ops_h__

#include <stdint.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "mrtrix.h"

namespace MR {
  namespace File {
#ifndef DOXYGEN_SHOULD_SKIP_THIS
    namespace {
      inline char random_char ()
      {
        char c = rand () % 62;
        if (c < 10) return (c+48);
        if (c < 36) return (c+55);
        return (c+61);
      }
    }
#endif /* DOXYGEN_SHOULD_SKIP_THIS */



    inline void create (const std::string& filename, off64_t size = 0) {
      int fid = open64 (filename.c_str(), O_CREAT | O_RDWR | O_EXCL, 0755);
      if (fid < 0) throw Exception ("error creating file \"" + filename + "\": " + strerror(errno));

      if (size) size = ftruncate64 (fid, size);
      close (fid);
      if (size) throw Exception ("WARNING: cannot resize file \"" + filename + "\": " + strerror(errno));
    }




    inline void resize (const std::string& filename, off64_t size)
    {
      debug ("resizing file \"" + filename + "\" to " + str (size) + "...");

      int fd = open64 (filename.c_str(), O_RDWR, 0755);
      if (fd < 0) throw Exception ("error opening file \"" + filename + "\" for resizing: " + strerror(errno));
      int status = ftruncate64 (fd, size);
      close (fd);
      if (status) throw Exception ("cannot resize file \"" + filename + "\": " + strerror(errno));
    }





    inline std::string scratch (off64_t size, const char* suffix = NULL) 
    {
      assert (size);
      debug ("creating scratch file of size " + str(size));

      std::string filename (TMPFILE_ROOT"XXXXXX.");
      filename += suffix ? suffix : "tmp"; 

      int fid;
      do {
        for (int n = 0; n < 6; n++) 
          filename[TMPFILE_ROOT_LEN+n] = random_char();
      } while ((fid = open64 (filename.c_str(), O_CREAT | O_RDWR | O_EXCL, 0755)) < 0);

      int status = ftruncate64 (fid, size);
      close (fid);
      if (status) throw Exception ("cannot resize file \"" + filename + "\": " + strerror(errno));
      return (filename);
    }

  }
}

#endif

