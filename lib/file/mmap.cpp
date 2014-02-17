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


#include <fcntl.h>
#include <unistd.h>
#include <zlib.h>

#ifdef MRTRIX_WINDOWS
#include <windows.h>
#else
#include <sys/mman.h>
#endif

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
      DEBUG (std::string (readwrite ? "creating RAM buffer for" : "memory-mapping" ) + " file \"" + Entry::name + "\"...");

      struct stat sbuf;
      if (stat (Entry::name.c_str(), &sbuf))
        throw Exception ("cannot stat file \"" + Entry::name + "\": " + strerror (errno));

      mtime = sbuf.st_mtime;


      if (msize < 0) 
        msize = sbuf.st_size - start;
      else if (start + msize > sbuf.st_size) 
        throw Exception ("file \"" + Entry::name + "\" is smaller than expected");

      if (readwrite) {
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
        DEBUG ("file \"" + Entry::name + "\" held in RAM at " + str ( (void*) first) + ", size " + str (msize));
      }
      else {

        if ( (fd = open (Entry::name.c_str(), O_RDONLY, 0644)) < 0)
          throw Exception ("error opening file \"" + Entry::name + "\": " + strerror (errno));

        try {
#ifdef MRTRIX_WINDOWS
          HANDLE handle = CreateFileMapping ( (HANDLE) _get_osfhandle (fd), NULL,
              PAGE_READONLY, 0, start + msize, NULL);
          if (!handle) throw 0;
          addr = static_cast<uint8_t*> (MapViewOfFile (handle, FILE_MAP_READ, 0, 0, start + msize));
          if (!addr) throw 0;
          CloseHandle (handle);
#else
          addr = static_cast<uint8_t*> (mmap ( (char*) 0, start + msize,
                PROT_READ, MAP_PRIVATE, fd, 0));
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
    }





    MMap::~MMap()
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
          std::ofstream out (Entry::name.c_str(), std::ios::in | std::ios::out | std::ios::binary);
          if (!out) 
            throw Exception ("failed to open file \"" + Entry::name + "\": " + strerror (errno));
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


