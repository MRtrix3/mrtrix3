/*
    Copyright 2009 Brain Research Institute, Melbourne, Australia

    Written by Robert E. Smith, 01/08/13.

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

#ifndef __image_sparse_value_h__
#define __image_sparse_value_h__


#include <cassert>
#include <cstring>


namespace MR
{
  namespace Image
  {
    namespace Sparse
    {


    // Provides access to sparse elements using [] operator - returns a reference or const reference of relevant class
    //   (the back-end of this is dealt with by the Handler::Sparse class)
    // The Value class never stores a local copy of any details regarding sparse data; on sparse data write,
    //   the memory-mapped or RAM-allocated region may move, which would invalidate such references

    template <class Voxel> class Value
    {
      public:

        Value (Voxel& parent) :
            V (parent) { }


        typedef typename Voxel::value_type value_type;
        typedef typename Voxel::sparse_data_type sparse_data_type;


        uint32_t size() const { return V.get_handler().get_numel (get_value()); }



        void set_size (const uint32_t n)
        {
          // Handler allocates new memory if necessary, and sets the relevant number of elements flag in the sparse image data
          // It returns the file offset necessary to access the relevant memory, so update the raw image value accordingly
          set_value (V.get_handler().set_numel (get_value(), n));
        }


        // Handler is responsible for bounds checking
        sparse_data_type& operator[] (const size_t i)
        {
          uint8_t* const ptr = V.get_handler().get (V.value(), i);
          return *(reinterpret_cast<sparse_data_type* const>(ptr));
        }
        const sparse_data_type& operator[] (const size_t i) const
        {
          const uint8_t* const ptr = V.get_handler().get (V.value(), i);
          return *(reinterpret_cast<const sparse_data_type* const>(ptr));
        }


        // This should provide image copying capability using the relevant templated functions
        Value& operator= (const Value& that) {
          set_size (that.size());
          for (uint32_t i = 0; i != size(); ++i)
            (*this)[i] = that[i];
          return *this;
        }


        // Need to explicitly clear any data on image creation
        // This is kind of an unsolved problem; when a new sparse image is created, the raw image data
        //   contains uninitialised values, which could cause a segfault if dereferenced.
        void zero() { set_value (0); }


        friend std::ostream& operator<< (std::ostream& stream, const Value& value) {
          stream << "Position [ ";
          for (size_t n = 0; n < value.V.ndim(); ++n)
            stream << value.V[n] << " ";
          stream << "], offset = " << value.get_value() << ", " << value.size() << " elements";
          return stream;
        }


      private:
        Voxel& V;


        value_type get_value () const { 
          return V.get_value();
        }

        operator value_type () const {
          return get_value();
        }

        // Forbid external direct access to raw image data (except for nulling using the zero() function)
        value_type set_value (value_type value) {
          V.set_value (value);
          return value;
        }


        // In the context of sparse data, this would make multiple voxels point to the same data
        // Not really something that should be supported...
        value_type operator= (value_type value) { assert (0); return value; }


    };




    }
  }
}

#endif

