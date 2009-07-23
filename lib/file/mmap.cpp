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
 

    11-07-2008 J-Donald Tournier <d.tournier@brain.org.au>
    * fixed TMPFILE_ROOT_LEN - now set to 7
    
*/

#include <fcntl.h>
#include <unistd.h>

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

    const off64_t MMap::pagesize = sysconf (_SC_PAGESIZE);

    MMap::MMap (const std::string& fname, bool readonly, off64_t from, off64_t to) :
      fd (-1), 
      filename (fname),
      addr (NULL), 
      fsize (0), 
      start (from),
      end (to),
      read_only (readonly),
      mtime (0)
    {
      assert (fname.size());
      assert (from < to);

      debug ("preparing memory-mapping for file \"" + fname + "\"");

      struct stat64 sbuf;
      if (stat64 (filename.c_str(), &sbuf)) 
        throw Exception ("cannot stat file \"" + filename + "\": " + strerror(errno));

      mtime = sbuf.st_mtime;
      fsize = sbuf.st_size;

      set_range (from, to); 
    }




    MMap::~MMap ()
    {
      unmap(); 

      // need to update:
      if (read_only && Path::is_temporary (filename)) {
        debug ("deleting file \"" + filename + "\"...");
        if (unlink (filename.c_str())) 
          error ("WARNING: error deleting file \"" + filename + "\": " + strerror(errno));
      }
    }


    void MMap::map()
    {
      if (addr) return;
      if (!is_ready()) throw Exception ("attempt to map file \"" + filename + "\" using invalid mmap!");

      if (Path::has_suffix (filename, ".gz")) {
      }
      else {
        if (end > fsize) throw Exception ("file \"" + filename + "\" is smaller than expected");

        off64_t mstart = pagesize * off64_t (start / pagesize);
        if ((fd = open64 (filename.c_str(), (read_only ? O_RDONLY : O_RDWR), 0755)) < 0) 
          throw Exception ("error opening file \"" + filename + "\": " + strerror(errno));

        try {
#ifdef WINDOWS
          // TODO: fix this for windows
          HANDLE handle = CreateFileMapping ((HANDLE) _get_osfhandle(fd), NULL, 
              (read_only ? PAGE_READONLY : PAGE_READWRITE), 0, end-mstart, NULL);
          if (!handle) throw 0;
          mapping_addr = static_cast<uint8_t*> (MapViewOfFile (handle, (read_only ? FILE_MAP_READ : FILE_MAP_ALL_ACCESS), 0, 0, end-mstart));
          if (!mapping_addr) throw 0;
          CloseHandle (handle);
#else 
          mapping_addr = static_cast<uint8_t*> (mmap64((char*)0, end-mstart, (read_only ? PROT_READ : PROT_READ | PROT_WRITE), MAP_SHARED, fd, mstart));
          if (mapping_addr == MAP_FAILED) throw 0;
#endif
          addr = mapping_addr + start - mstart;
          debug ("file \"" + filename + "\" mapped at " + str (addr) 
              + ", size " + str (size()) 
              + " (read-" + ( read_only ? "only" : "write" ) + ")"); 
        }
        catch (...) {
          close (fd);
          addr = NULL;
          throw Exception ("memmory-mapping failed for file \"" + filename + "\": " + strerror(errno));
        }
      }
    }





    void MMap::unmap()
    {
      if (!addr) return;

      debug ("unmapping file \"" + filename + "\"");
      off64_t mstart = pagesize * off64_t (start / pagesize);

#ifdef WINDOWS
      if (!UnmapViewOfFile ((LPVOID) mapping_addr))
#else 
        if (munmap (mapping_addr, end-mstart))
#endif
          error ("error unmapping file \"" + filename + "\": " + strerror(errno));

      close (fd);
      fd = -1;
      addr = mapping_addr = NULL;
    }









    bool MMap::changed () const
    { 
      assert (fd >= 0);
      struct stat64 sbuf;
      if (fstat64 (fd, &sbuf)) return (false);
      if (off64_t (fsize) != sbuf.st_size) return (true);
      if (mtime != sbuf.st_mtime) return (true);
      return (false);
    }




  }
}


