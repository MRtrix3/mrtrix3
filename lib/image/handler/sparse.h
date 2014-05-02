/*
   Copyright 2009 Brain Research Institute, Melbourne, Australia

   Written by Robert E. Smith, 31/07/13.

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

#ifndef __image_handler_sparse_h__
#define __image_handler_sparse_h__

#include <cassert>
#include <cstring>
#include <fstream>
#include <typeinfo>

#include "debug.h"
#include "ptr.h"
#include "file/config.h"
#include "file/mmap.h"
#include "file/utils.h"
#include "image/handler/base.h"
#include "image/handler/default.h"



namespace MR
{
  namespace Image
  {
    namespace Handler
    {




      // A quick description of how the sparse image data are currently stored:
      // * The data are either after the image data within the same file if extension is .msf, or
      //     in a separate file with the .sdat extension if the image extension if .msh
      // * The image header must store the fields defined in lib/image/sparse/key.h
      //     These are currently verified on construction of the BufferSparse class. This proved to
      //     be simpler than trying to verify class matching on every interaction with the handler
      //     using templated functions.
      // * The raw image data consists of unsigned 64-bit integer values. These values correspond to
      //     an offset from the start of the sparse data (wherever that may be) to the sparse data
      //     stored for that particular voxel.
      // * Wherever sparse data for a voxel is stored, the data begins with a single unsigned 32-bit
      //     integer, which encodes the number of elements in that voxel. The following data is then
      //     a raw memory dump of that many instances of the relevant class type.
      // * When a sparse image is created for writing, a single unsigned 32-bit integer value of 0
      //     is written at the start of the sparse data. This is done so that uninitialised voxels
      //     can have their raw image value set to 0, and if they are dereferenced, the handler will
      //     indicate that there are zero elements for that voxel.
      //     - Note however that nulling all image data on image construction is NOT currently handled
      //       by either the Handler::Sparse class nor the Image::BufferSparse class. The programmer
      //       is responsible for this initialisation for the time being.
      // * The handler does not attempt any type of endianness conversion of the sparse data, so the
      //     systems that read/write the image files must have the same endianness. Since this can't
      //     be determined from the sparse data alone, the relevant Image::Format instead enforces
      //     the endianness of the image data to be native, and assumes that the sparse data has
      //     the same endianness. If the endianness does not match, the file won't open.



    // FIXME Currently, this is using the Base::writable flag to initialise MMap
    // Within the MMap constructor, this influences whether or not the data is allocated in RAM
    //   using new[], or memory-mapped as read-only
    // What is in fact required for this class is a memory-mapping that is read-write and does NOT use a
    //   RAM buffer
    // For now, this will simply mean that sparse data greater than available RAM cannot be written
    // Long-term, it would be preferable to enhance the MMap constructor; have separate flags for whether
    //   the memory should be allocated in RAM beforehand, and if it is not, whether the memory-mapped
    //   region should be read-write. Additionally could have a config file entry that can be set on HPC
    //   systems / systems with distributed storage that forbids creation of readwrite memory-mapped
    //   regions and therefore forces use of a RAM buffer.


      class Sparse : public Default
      {
        public:
          Sparse (Default& handler, const std::string&, const size_t, const File::Entry entry);

          ~Sparse () { close(); }


          // Find the number of elements in a particular voxel based on the file offset
          uint32_t get_numel (const uint64_t) const;

          // Request memory location for new sparse data to be written - also ensures that the sparse data buffer is
          //   sufficiently large to contain the new information
          // Receives current offset value for that voxel, and the desired number of elements
          // Return value is the offset from the start of the sparse data
          uint64_t set_numel (const uint64_t, const uint32_t);

          // Return a pointer to an element in a voxel
          uint8_t* get (const uint64_t, const size_t) const;


          const std::string& get_class_name() const { return class_name; }
          size_t       get_class_size() const { return class_size; }


        protected:
          virtual void load ();
          virtual void unload ();

          const std::string class_name;
          const size_t class_size;
          const File::Entry file;
          uint64_t data_end;
          Ptr<File::MMap> mmap;


          uint64_t size() const { return (mmap ? mmap->size() : 0); }

          // Convert a file position offset (as read from the image data) to a pointer to the relevant sparsely-stored data
          uint8_t* off2mem (const uint64_t i) const { assert (mmap); return (mmap->address() + i); }


      };



    }
  }
}

#endif


