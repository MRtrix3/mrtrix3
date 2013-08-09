/*
    Copyright 2008 Brain Research Institute, Melbourne, Australia

    Written by J-Donald Tournier, 23/05/09.

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

#ifndef __image_sparse_buffer_sparse_h__
#define __image_sparse_buffer_sparse_h__

#include <typeinfo>

#include "image/buffer.h"
#include "image/loop.h"
#include "image/handler/sparse.h"
#include "image/sparse/keys.h"


namespace MR
{
  namespace Image
  {


    namespace Sparse
    {
      template <class SparseDataType> class Voxel;
    }



    // A convenience class for wrapping access to sparse images
    // Main reason for having this class is to clarify the code responsible for accessing sparse images
    // In particular it provides the ::voxel_type typedef
    // Also: Any attempt to unify this concept of image data storage will likely also involve
    //   generalisation of the other Buffer offshoot classes (BufferPreload, BufferScratch)
    template <typename SparseDataType>
      class BufferSparse : public Buffer<uint64_t>
    {
      public:
        BufferSparse (const std::string& image_name, bool readwrite = false) :
            Buffer<uint64_t> (image_name, readwrite) { check(); }

        BufferSparse (const Header& header, bool readwrite = false) :
            Buffer<uint64_t> (header, readwrite) { check(); }

        BufferSparse (const BufferSparse<SparseDataType>& that) :
            Buffer<uint64_t> (that) { }

        BufferSparse (const std::string& image_name, const Header& template_header) :
            Buffer<uint64_t> (image_name, template_header) { check(); }


        typedef uint64_t value_type;
        typedef SparseDataType sparse_data_type;
        typedef Image::Sparse::Voxel<sparse_data_type> voxel_type;



      protected:
        template <class Set> 
          BufferSparse& operator= (const Set& H) { assert (0); return *this; }


        // Don't permit constructing from another BufferSparse using a different type
        template <typename OtherSparseType>
          BufferSparse (const BufferSparse<OtherSparseType>& buffer) : Buffer<uint64_t> (buffer) { assert (0); }


        void check()
        {
          if (!handler_)
            throw Exception ("cannot create sparse image buffer for image with no handler");
          if (typeid (*handler_) != typeid (Handler::Sparse))
            throw Exception ("cannot create sparse image buffer for accessing non-sparse image");
          // Use the header information rather than trying to access this from the handler
          std::map<std::string, std::string>::const_iterator name_it = find(Image::Sparse::name_key);
          if (name_it == end())
            throw Exception ("cannot create sparse image buffer without knowledge of underlying class type in the image header");
          const std::string& class_name = name_it->second;
          if (str(typeid(SparseDataType).name()) != class_name)
            throw Exception ("class type of sparse image buffer does not match that in image header");
          std::map<std::string, std::string>::const_iterator size_it = find(Image::Sparse::size_key);
          if (size_it == end())
            throw Exception ("cannot create sparse image buffer without knowledge of underlying class size in the image header");
          const size_t class_size = to<size_t>(size_it->second);
          if (sizeof(SparseDataType) != class_size)
            throw Exception ("class size of sparse image buffer does not match that in image header");
          DEBUG ("Sparse image buffer verified for accessing image " + name() + " using type " + str(typeid(SparseDataType).name()));
        }


    };



  }
}

#endif



