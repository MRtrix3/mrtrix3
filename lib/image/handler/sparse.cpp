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


#include "image/handler/sparse.h"



namespace MR
{
  namespace Image
  {
    namespace Handler
    {


      Sparse::Sparse (Default& handler, const std::string& sparse_class, const size_t sparse_size, const File::Entry entry) :
          Default (handler),
          class_name (sparse_class),
          class_size (sparse_size),
          file (entry),
          data_end (0) { }


      void Sparse::load()
      {

        Default::load();

        std::fstream stream (file.name.c_str(), std::ios_base::in | std::ios_base::binary);
        stream.seekg (0, std::ios::end);
        const uint64_t file_size = stream.tellg();
        stream.close();
        const uint64_t current_sparse_data_size = file_size - file.start;

        if (current_sparse_data_size) {

          mmap = new File::MMap (file, Base::writable, true, current_sparse_data_size);
          data_end = current_sparse_data_size;

        } else if (Base::writable) {

          // Default = initialise 16MB, this is enough to store whole-brain fixel data at 2.5mm resolution
          const uint64_t init_sparse_data_size = File::Config::get_int ("SparseDataInitialSize", 16777216);
          const size_t new_file_size = file.start + init_sparse_data_size;
          DEBUG ("Initialising output sparse data file " + file.name + ": new file size " + str(new_file_size) + " (" + str(init_sparse_data_size) + " of which is initial sparse data buffer)");
          File::resize (file.name, new_file_size);
          mmap = new File::MMap (file, Base::writable, false, init_sparse_data_size);

          // Writes a single uint32_t(0) to the start of the sparse data region
          // Any voxel that has its value initialised to 0 will point here, and therefore dereferencing of any
          //   such voxel will yield a SParse::Value with zero elements
          uint32_t zero (0);
          memcpy (off2mem(0), &zero, sizeof(uint32_t));

          data_end = sizeof(uint32_t);

        }

      }



      void Sparse::unload()
      {

        Default::unload();

        const uint64_t truncate_file_size = (data_end == size()) ? 0 : file.start + data_end;
        // Null the excess data before closing the memory map to prevent std::ofstream from giving an error
        //   (writing uninitialised values)
        memset (off2mem(data_end), 0x00, size() - data_end);
        mmap = NULL;

        if (truncate_file_size) {
          DEBUG ("truncating sparse image data file " + file.name + " to " + str(truncate_file_size) + " bytes");
          File::resize (file.name, truncate_file_size);
        }

      }







      uint32_t Sparse::get_numel (const uint64_t offset) const
      {
        return *(reinterpret_cast<uint32_t*>(off2mem(offset)));
      }



      uint64_t Sparse::set_numel (const uint64_t old_offset, const uint32_t numel)
      {

        assert (Base::writable);

        // Before allocating new memory, verify that the current offset does not point to
        //   a voxel that has more elements than is required
        if (old_offset) {

          assert (old_offset < data_end);

          const uint32_t existing_numel = get_numel (old_offset);
          if (existing_numel >= numel) {

            // Set the new number of elements, clear the unwanted data, and return
            memcpy (off2mem(old_offset), &numel, sizeof(uint32_t));
            memset (off2mem(old_offset) + sizeof(uint32_t) + (numel * class_size), 0x00, ((existing_numel - numel) * class_size));
            // If this voxel is being cleared (i.e. no elements), rather than returning a pointer to a voxel with
            //   zero elements, instead return a pointer to the start of the sparse data, where there's a single
            //   uint32_t(0) which can be used for any and all voxels with zero elements
            return numel ? old_offset : 0;

          } else {

            // Existing memory allocation for this voxel is not sufficient; erase it
            // Note that the handler makes no attempt at re-using this memory later; new data is just concatenated
            //   to the end of the buffer
            memset (off2mem(old_offset), 0x00, sizeof(uint32_t) + (existing_numel * class_size));

          }
        }

        // Don't allocate memory for voxels with no sparse data; just point them all to the start of the
        //   sparse data where theres a single uint32_t(0)
        if (!numel)
          return 0;

        const int64_t requested_size = sizeof (uint32_t) + (numel * class_size);
        if (data_end + requested_size > size()) {

          // size() should never be empty if data is being written...
          assert (size());
          uint64_t new_sparse_data_size = 2 * size();

          // Cover rare case where a huge memory request occurs early
          while (new_sparse_data_size < data_end + requested_size)
            new_sparse_data_size *= 2;

          // Memory error arises due to the uninitialised data between the end of the sparse data and the end
          //   of the file at its current size being written using std::ofstream
          // Therefore, explicitly null all data that hasn't already been written
          //   (this does not prohibit the use of this memory for subsequent sparse data though)
          memset (off2mem(data_end), 0x00, size() - data_end);

          mmap = NULL;

          const size_t new_file_size = file.start + new_sparse_data_size;
          DEBUG ("Resizing sparse data file " + file.name + ": new file size " + str(new_file_size) + " (" + str(new_sparse_data_size) + " of which is for sparse data)");
          File::resize (file.name, new_file_size);
          mmap = new File::MMap (file, Base::writable, true, new_sparse_data_size);

        }

        // Write the uint32_t indicating the number of elements in this voxel
        memcpy (off2mem(data_end), &numel, sizeof(uint32_t));

        // The return value is the offset from the beginning of the sparse data
        const uint64_t ret = data_end;
        data_end += requested_size;

        return ret;
      }




      uint8_t* const Sparse::get (const uint64_t voxel_offset, const size_t index) const
      {
        assert (index < get_numel (voxel_offset));
        const uint64_t offset = sizeof(uint32_t) + (index * class_size);
        assert (voxel_offset + offset + class_size <= data_end);
        uint8_t* const ptr = off2mem(voxel_offset) + offset;
        return ptr;
      }




    }
  }
}


