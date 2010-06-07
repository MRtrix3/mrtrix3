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

#include <fcntl.h>
#include <unistd.h>
#include <zlib.h>

#ifdef WINDOWS
#include <windows.h>
#else 
#include <sys/mman.h>
#endif

#include "file/path.h"
#include "file/mmap.h"
#include "file/config.h"

namespace MR {
  namespace File {

    void MMap::map()
    {
      debug ("memory-mapping file \"" + Entry::name + "\"...");

      struct stat64 sbuf;
      if (stat64 (Entry::name.c_str(), &sbuf)) 
        throw Exception ("cannot stat file \"" + Entry::name + "\": " + strerror(errno));

      mtime = sbuf.st_mtime;

      if (start + msize > sbuf.st_size) throw Exception ("file \"" + Entry::name + "\" is smaller than expected");
      if (msize < 0) msize = sbuf.st_size - start;

      if ((fd = open64 (Entry::name.c_str(), (readwrite ? O_RDWR : O_RDONLY), 0644)) < 0) 
        throw Exception ("error opening file \"" + Entry::name + "\": " + strerror(errno));

      try {
#ifdef WINDOWS
        HANDLE handle = CreateFileMapping ((HANDLE) _get_osfhandle(fd), NULL, 
            (readwrite ? PAGE_READWRITE : PAGE_READONLY), 0, start + msize, NULL);
        if (!handle) throw 0;
        addr = static_cast<uint8_t*> (MapViewOfFile (handle, (readwrite ? FILE_MAP_ALL_ACCESS : FILE_MAP_READ), 0, 0, start + msize));
        if (!addr) throw 0;
        CloseHandle (handle);
#else 
        addr = static_cast<uint8_t*> (mmap64((char*)0, start + msize, 
              (readwrite ? PROT_READ | PROT_WRITE : PROT_READ), MAP_SHARED, fd, 0));
        if (addr == MAP_FAILED) throw 0;
#endif
      }
      catch (...) {
        close (fd);
        addr = NULL;
        throw Exception ("memmory-mapping failed for file \"" + Entry::name + "\": " + strerror(errno));
      }

      debug ("file \"" + Entry::name + "\" mapped at " + str ((void*) addr) + ", size " + str (msize) 
          + " (read-" + ( readwrite ? "write" : "only" ) + ")"); 
    }





    MMap::~MMap()
    {
      if (!addr) return;
      debug ("unmapping file \"" + Entry::name + "\"");
#ifdef WINDOWS
      if (!UnmapViewOfFile ((LPVOID) addr))
#else 
        if (munmap (addr, msize))
#endif
          error ("error unmapping file \"" + Entry::name + "\": " + strerror(errno));
      close (fd);
    }






    bool MMap::changed () const
    { 
      assert (fd >= 0);
      struct stat64 sbuf;
      if (fstat64 (fd, &sbuf)) return (false);
      if (off64_t (msize) != sbuf.st_size) return (true);
      if (mtime != sbuf.st_mtime) return (true);
      return (false);
    }




  }
}


