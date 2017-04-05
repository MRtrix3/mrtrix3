/*
 * Copyright (c) 2008-2016 the MRtrix3 contributors
 * 
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/
 * 
 * MRtrix is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * 
 * For more details, see www.mrtrix.org
 * 
 */

#ifndef __sparse_image_h__
#define __sparse_image_h__

#include <typeinfo>

#include "image.h"
#include "header.h"
#include "image_io/sparse.h"
#include "sparse/keys.h"

#ifndef __image_h__
#error File that #includes "sparse/image.h" must explicitly #include "image.h" beforehand
#endif


namespace MR
{
  namespace Sparse
  {

    template <typename DataType>
      class Value {
        public:
          Value (::MR::Image<uint64_t>& offsets, ImageIO::Sparse& io) : offsets (offsets), io (io) { } 

        uint32_t size() const { return io.get_numel (offsets.value()); }

        void set_size (const uint32_t n)
        {
          // Handler allocates new memory if necessary, and sets the relevant number of elements flag in the sparse image data
          // It returns the file offset necessary to access the relevant memory, so update the raw image value accordingly
          offsets.value() = (io.set_numel (offsets.value(), n));
        }

        // Handler is responsible for bounds checking
        DataType& operator[] (const size_t i)
        {
          uint8_t* const ptr = io.get (offsets.value(), i);
          return *(reinterpret_cast<DataType* const>(ptr));
        }
        const DataType& operator[] (const size_t i) const
        {
          const uint8_t* const ptr = io.get (offsets.value(), i);
          return *(reinterpret_cast<const DataType* const>(ptr));
        }


        // This should provide image copying capability using the relevant templated functions
        Value& operator= (const Value& that) {
          set_size (that.size());
          for (uint32_t i = 0; i != size(); ++i)
            (*this)[i] = that[i];
          return *this;
        }


        friend std::ostream& operator<< (std::ostream& stream, const Value& value) {
          stream << "Position [ ";
          for (size_t n = 0; n < value.offsets.ndim(); ++n)
            stream << value.offsets[n] << " ";
          stream << "], offset = " << value.offsets.value() << ", " << value.size() << " elements";
          return stream;
        }



        protected:
          ::MR::Image<uint64_t>& offsets;
          ImageIO::Sparse& io;
      };







    // A convenience class for wrapping access to sparse images
    template <typename DataType>
      class Image : public ::MR::Image<uint64_t>
    {
      public:
        Image (const std::string& image_name) :
          ::MR::Image<uint64_t> (::MR::Image<uint64_t>::open (image_name)), io (nullptr) { check(); }

        Image (Header& header) :
          ::MR::Image<uint64_t> (header.get_image<uint64_t>()), io (nullptr) { check(); }

        Image (const Image<DataType>& that) = default;

        Image (const std::string& image_name, const Header& template_header) :
          ::MR::Image<uint64_t> (::MR::Image<uint64_t>::create (image_name, template_header)), io (nullptr) { check(); }

        using value_type = uint64_t;
        using sparse_data_type = DataType;

        Value<DataType> value () { return { *this, *io }; }
        const Value<DataType> value () const { return { *this, *io }; }

      protected:
        ImageIO::Sparse* io;

        void check()
        {
          if (!buffer || !buffer->get_io())
            throw Exception ("cannot create sparse image for image with no handler");
          ImageIO::Base* ptr = buffer->get_io();
          if (typeid (*ptr) != typeid (ImageIO::Sparse))
            throw Exception ("cannot create sparse image to access non-sparse data");
          // Use the header information rather than trying to access this from the handler
          std::map<std::string, std::string>::const_iterator name_it = keyval().find (Sparse::name_key);
          if (name_it == keyval().end())
            throw Exception ("cannot create sparse image without knowledge of underlying class type in the image header");
          // TODO temporarily disabled this to allow updated_syntax tests to pass with files generated with master branch.
//          const std::string& class_name = name_it->second;
//          if (str(typeid(DataType).name()) != class_name)
//            throw Exception ("class type of sparse image buffer (" + str(typeid(DataType).name()) + ") does not match that in image header (" + class_name + ")");
          std::map<std::string, std::string>::const_iterator size_it = keyval().find (Sparse::size_key);
          if (size_it == keyval().end())
            throw Exception ("cannot create sparse image without knowledge of underlying class size in the image header");
          const size_t class_size = to<size_t>(size_it->second);
          if (sizeof(DataType) != class_size)
            throw Exception ("class size of sparse image does not match that in image header");
          io = reinterpret_cast<ImageIO::Sparse*> (buffer->get_io());
          DEBUG ("Sparse image verified for accessing " + name() + " using type " + str(typeid(DataType).name()));
        }


    };



  }
}

#endif



