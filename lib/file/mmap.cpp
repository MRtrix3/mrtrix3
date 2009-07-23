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

#include "file/gz.h"


namespace MR {
  namespace File {

    namespace {

      inline char random_char ()
      {
        char c = rand () % 62;
        if (c < 10) return (c+48);
        if (c < 36) return (c+55);
        return (c+61);
      }

    }






    MMap::MMap (const std::string& fname, off64_t desired_size_if_inexistant, off64_t data_offset, const char* suffix) :
      fd (-1), 
      addr (NULL), 
      msize (0), 
      byte_offset (data_offset), 
      read_only (true),
      delete_after (false),
      mtime (0)
    {
      if (fname.size()) {
        debug ("preparing memory-mapping for file \"" + fname + "\"");

        filename = fname;
        struct stat64 sbuf;
        if (stat64 (filename.c_str(), &sbuf)) {

          if (errno != ENOENT) 
            throw Exception ("cannot stat file \"" + filename + "\": " + strerror(errno));

          if (desired_size_if_inexistant == 0) 
            throw Exception ("cannot access file \"" + filename + "\": " + strerror(errno));

          int fid = open64 (filename.c_str(), O_CREAT | O_RDWR | O_EXCL, 0755);
          if (fid < 0) throw Exception ("error creating file \"" + filename + "\": " + strerror(errno));

          int status = ftruncate64 (fid, desired_size_if_inexistant);
          close (fid);
          if (status) throw Exception ("WARNING: cannot resize file \"" + filename + "\": " + strerror(errno));

          read_only = false;
          msize = desired_size_if_inexistant;
        }
        else {
          if (desired_size_if_inexistant) 
            throw Exception ("cannot create file \"" + filename + "\": it already exists");

          msize = sbuf.st_size;
          mtime = sbuf.st_mtime;
        }

      }
      else {

        if (!desired_size_if_inexistant) throw Exception ("cannot create empty scratch file");

        debug ("creating and mapping scratch file");

        assert (suffix);
        filename = std::string (TMPFILE_ROOT) + "XXXXXX." + suffix; 

        int fid;
        do {
          for (int n = 0; n < 6; n++) 
            filename[TMPFILE_ROOT_LEN+n] = random_char();
        } while ((fid = open64 (filename.c_str(), O_CREAT | O_RDWR | O_EXCL, 0755)) < 0);


        int status = ftruncate64 (fid, desired_size_if_inexistant);
        close (fid);
        if (status) throw Exception ("cannot resize file \"" + filename + "\": " + strerror(errno));

        msize = desired_size_if_inexistant;
        read_only = false;
      }
    }




    MMap::~MMap ()
    {
      unmap(); 
      if (delete_after) {
        debug ("deleting file \"" + filename + "\"...");
        if (unlink (filename.c_str())) 
          error ("WARNING: error deleting file \"" + filename + "\": " + strerror(errno));
      }
    }


    void MMap::map()
    {
      if (msize == 0) throw Exception ("attempt to map file \"" + filename + "\" using invalid mmap!");
      if (addr) return;

      if ((fd = open64 (filename.c_str(), (read_only ? O_RDONLY : O_RDWR), 0755)) < 0) 
        throw Exception ("error opening file \"" + filename + "\": " + strerror(errno));

      try {
#ifdef WINDOWS
        HANDLE handle = CreateFileMapping ((HANDLE) _get_osfhandle(fd), NULL, 
            (read_only ? PAGE_READONLY : PAGE_READWRITE), 0, msize, NULL);
        if (!handle) throw 0;
        addr = static_cast<uint8_t*> (MapViewOfFile (handle, (read_only ? FILE_MAP_READ : FILE_MAP_ALL_ACCESS), 0, 0, msize));
        if (!addr) throw 0;
        CloseHandle (handle);
#else 
        addr = static_cast<uint8_t*> (mmap64((char*)0, msize, (read_only ? PROT_READ : PROT_READ | PROT_WRITE), MAP_SHARED, fd, 0));
        if (addr == MAP_FAILED) throw 0;
#endif
        debug ("file \"" + filename + "\" mapped at " + str (addr) 
            + ", size " + str (msize) 
            + " (read-" + ( read_only ? "only" : "write" ) + ")"); 
      }
      catch (...) {
        close (fd);
        addr = NULL;
        throw Exception ("memmory-mapping failed for file \"" + filename + "\": " + strerror(errno));
      }
    }





    void MMap::unmap()
    {
      if (!addr) return;

      debug ("unmapping file \"" + filename + "\"");

#ifdef WINDOWS
      if (!UnmapViewOfFile ((LPVOID) addr))
#else 
        if (munmap (addr, msize))
#endif
          error ("error unmapping file \"" + filename + "\": " + strerror(errno));

      close (fd);
      fd = -1;
      addr = NULL;
    }









    void MMap::resize (off64_t new_size)
    {
      debug ("resizing file \"" + filename + "\" to " + str (new_size) + "...");

      if (read_only) throw Exception ("attempting to resize read-only file \"" + filename + "\"");
      unmap();

      if ((fd = open64 (filename.c_str(), O_RDWR, 0755)) < 0) 
        throw Exception ("error opening file \"" + filename + "\" for resizing: " + strerror(errno));

      int status = ftruncate64 (fd, new_size);

      close (fd);
      fd = -1;
      if (status) throw Exception ("cannot resize file \"" + filename + "\": " + strerror(errno));

      msize = new_size;
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


