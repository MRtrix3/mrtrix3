/* Copyright (c) 2008-2017 the MRtrix3 contributors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, you can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * MRtrix is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * For more details, see http://www.mrtrix.org/.
 */


#include <fcntl.h>
#include <unistd.h>
#include <zlib.h>

#ifdef MRTRIX_MACOSX
# include <sys/param.h>
# include <sys/mount.h>
#elif !defined(MRTRIX_WINDOWS)
# include <sys/vfs.h> 
#endif

#ifdef MRTRIX_WINDOWS
#include <windows.h>
#else
#include <sys/mman.h>
#endif

#include "file/ofstream.h"
#include "file/path.h"
#include "file/mmap.h"
#include "file/config.h"

#include "debug.h"

namespace MR
{
  namespace File
  {

    MMap::MMap (const Entry& entry, bool readwrite, bool preload, int64_t mapped_size) :
      Entry (entry), addr (NULL), first (NULL), msize (mapped_size), readwrite (readwrite)
    {
      DEBUG ("memory-mapping file \"" + Entry::name + "\"...");

      struct stat sbuf;
      if (stat (Entry::name.c_str(), &sbuf))
        throw Exception ("cannot stat file \"" + Entry::name + "\": " + strerror (errno));

      mtime = sbuf.st_mtime;


      if (msize < 0) 
        msize = sbuf.st_size - start;
      else if (start + msize > sbuf.st_size) 
        throw Exception ("file \"" + Entry::name + "\" is smaller than expected");

      bool delayed_writeback = false;
      if (readwrite) {

#ifdef MRTRIX_WINDOWS
        const unsigned int length = 255;
        char root_path[length];
        if (GetVolumePathName (Entry::name.c_str(), root_path, length)) { // Returns non-zero on success

          const unsigned int code = GetDriveType (root_path);
          switch (code) {
            case 0: // DRIVE_UNKNOWN
              DEBUG ("cannot get filesystem information on file \"" + Entry::name + "\": " + strerror (errno));
              DEBUG ("  defaulting to delayed write-back");
              delayed_writeback = true;
              break;
            case 1: // DRIVE_NO_ROOT_DIR:
              DEBUG ("erroneous root path derived for file \"" + Entry::name + "\": " + strerror (errno));
              DEBUG ("  defaulting to delayed write-back");
              delayed_writeback = true;
              break;
            case 2: // DRIVE_REMOVABLE
              DEBUG ("Drive for file \"" + Entry::name + "\" detected as removable; using memory-mapping");
              break;
            case 3: // DRIVE_FIXED
              DEBUG ("Drive for file \"" + Entry::name + "\" detected as fixed; using memory-mapping");
              break;
            case 4: // DRIVE_REMOTE
              DEBUG ("Drive for file \"" + Entry::name + "\" detected as network - using delayed write-back");
              delayed_writeback = true;
              break;
            case 5: // DRIVE_CDROM
              DEBUG ("Drive for file \"" + Entry::name + "\" detected as CD-ROM - using delayed write-back");
              delayed_writeback = true;
              break;
            case 6: // DRIVE_RAMDISK
              DEBUG ("Drive for file \"" + Entry::name + "\" detected as RAM - using memory-mapping");
              break;
          }

        } else {
          DEBUG ("unable to query root drive path for file \"" + Entry::name + "\"; using delayed write-back");
          delayed_writeback = true;
        }
#else
        struct statfs fsbuf;
        if (statfs (Entry::name.c_str(), &fsbuf)) {
          DEBUG ("cannot get filesystem information on file \"" + Entry::name + "\": " + strerror (errno));
          DEBUG ("  defaulting to delayed write-back");
          delayed_writeback = true;
        }

        if (fsbuf.f_type == 0xff534d42 /* CIFS */|| fsbuf.f_type == 0x6969 /* NFS */ || 
            fsbuf.f_type == 0x65735546 /* FUSE */ || fsbuf.f_type == 0x517b /* SMB */
#ifdef MRTRIX_MACOSX
            || fsbuf.f_type == 0x0017 /* OSXFUSE */
#endif 
        ) {
          DEBUG ("\"" + Entry::name + "\" appears to reside on a networked filesystem - using delayed write-back");
          delayed_writeback = true;
        }
#endif

        if (delayed_writeback) {
          try {
            first = new uint8_t [msize];
            if (!first) throw 1;
          }
          catch (...) {
            throw Exception ("error allocating memory to hold mmap buffer contents");
          }

          if (preload) {
            CONSOLE ("preloading contents of mapped file \"" + Entry::name + "\"...");
            std::ifstream in (Entry::name.c_str(), std::ios::in | std::ios::binary);
            if (!in) 
              throw Exception ("failed to open file \"" + Entry::name + "\": " + strerror (errno));
            in.seekg (start, in.beg);
            in.read ((char*) first, msize);
            if (!in.good())
              throw Exception ("error preloading contents of file \"" + Entry::name + "\": " + strerror(errno));
          }
          else 
            memset (first, 0, msize);
          DEBUG ("file \"" + Entry::name + "\" held in RAM at " + str ( (void*) first) + ", size " + str (msize));

          return;
        }
      }

      // use regular memory-mapping:

      if ( (fd = open (Entry::name.c_str(), ( readwrite ? O_RDWR : O_RDONLY ), 0666)) < 0)
        throw Exception ("error opening file \"" + Entry::name + "\": " + strerror (errno));

      try {
#ifdef MRTRIX_WINDOWS
        HANDLE handle = CreateFileMapping ( (HANDLE) _get_osfhandle (fd), NULL,
            ( readwrite ? PAGE_READWRITE : PAGE_READONLY ), 0, start + msize, NULL);
        if (!handle) throw 0;
        addr = static_cast<uint8_t*> (MapViewOfFile (handle, ( readwrite ? FILE_MAP_ALL_ACCESS : FILE_MAP_READ ), 0, 0, start + msize));
        if (!addr) throw 0;
        CloseHandle (handle);
#else
        addr = static_cast<uint8_t*> (mmap ( (char*) 0, start + msize,
              ( readwrite ? PROT_WRITE | PROT_READ : PROT_READ ), MAP_SHARED, fd, 0));
        if (addr == MAP_FAILED) throw 0;
#endif
      }
      catch (...) {
        close (fd);
        addr = NULL;
        throw Exception ("memory-mapping failed for file \"" + Entry::name + "\": " + strerror (errno));
      }
      first = addr + start;

      DEBUG ("file \"" + Entry::name + "\" mapped at " + str ( (void*) addr) + ", size " + str (msize)
          + " (read-" + (readwrite ? "write" : "only") + ")");
    }





    MMap::~MMap() noexcept (false)
    {
      if (!first) return;
      if (addr) {
        DEBUG ("unmapping file \"" + Entry::name + "\"");
#ifdef MRTRIX_WINDOWS
        if (!UnmapViewOfFile ( (LPVOID) addr))
#else
          if (munmap (addr, msize))
#endif
            WARN ("error unmapping file \"" + Entry::name + "\": " + strerror (errno));
        close (fd);
      }
      else {
        if (readwrite) {
          INFO ("writing back contents of mapped file \"" + Entry::name + "\"...");
          File::OFStream out (Entry::name, std::ios::in | std::ios::out | std::ios::binary);
          out.seekp (start, out.beg);
          out.write ((char*) first, msize);
          if (!out.good())
            throw Exception ("error writing back contents of file \"" + Entry::name + "\": " + strerror(errno));
        }
        delete [] first;
      }
    }






    bool MMap::changed () const
    {
      assert (fd >= 0);
      struct stat sbuf;
      if (fstat (fd, &sbuf)) 
        return false;
      if (int64_t (msize) != sbuf.st_size) 
        return true;
      if (mtime != sbuf.st_mtime) 
        return true;
      return false;
    }




  }
}


