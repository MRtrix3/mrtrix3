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


#ifndef __file_mmap_h__
#define __file_mmap_h__

#include <iostream>
#include <cassert>
#include <stdint.h>

#include "types.h"
#include "file/entry.h"

namespace MR
{
  namespace File
  {

    class MMap : protected Entry
    {
      public:
        //! create a new memory-mapping to file in \a entry
        /*! map file in \a entry at the offset in \a entry. By default, the
         * file will be mapped read-only. If \a readwrite is set to true, a
         * write-back RAM buffer will be allocated to store the contents of the
         * file, and written back when the constructor is invoked. 
         *
         * By default, the contents of a file mapped read-write will be
         * preloaded into the RAM buffer. If the file has just been created, \a
         * preload can be set to false to prevent preloading its contents into
         * the buffer. 
         *
         * By default, the whole file is mapped. If \a mapped_size is
         * non-zero, then only the region of size \a mapped_size starting from
         * the byte offset specified in \a entry will be mapped. 
         */
        MMap (const Entry& entry, bool readwrite = false, bool preload = true, int64_t mapped_size = -1);
        ~MMap ();

        std::string name () const {
          return Entry::name;
        }
        int64_t size () const {
          return msize;
        }
        uint8_t* address() {
          return first;
        }
        const uint8_t* address() const {
          return first;
        }

        bool is_read_write () const {
          return readwrite;
        }
        bool changed () const;

        friend std::ostream& operator<< (std::ostream& stream, const MMap& m) {
          stream << "File::MMap { " << m.name() << " [" << m.fd << "], size: "
                 << m.size() << ", mapped " << (m.readwrite ? "RW" : "RO")
                 << " at " << (void*) m.address() << ", offset " << m.start << " }";
          return stream;
        }

      protected:
        int       fd;
        uint8_t*  addr;        /**< The address in memory where the file has been mapped. */
        uint8_t*  first;       /**< The address in memory to the start of the region of interest. */
        int64_t   msize;       /**< The size of the file. */
        time_t    mtime;       /**< The modification time of the file at the last check. */
        bool      readwrite;

        void map ();

      private:
        MMap (const MMap& mmap) : Entry (mmap), fd (0), addr (NULL), first (NULL), msize (0), mtime (0), readwrite (false) {
          assert (0);
        }
    };


  }
}

#endif

