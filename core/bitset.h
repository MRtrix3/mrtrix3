/* Copyright (c) 2008-2017 the MRtrix3 contributors.
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


#ifndef __bitset_h__
#define __bitset_h__


#include <atomic>
#include <cstdint>
#include "mrtrix.h"



namespace MR {



  //! a class for storing bitwise information
  /*! The BitSet class stores information in a bitwise fashion. Only a single
   * bit of memory is used for each bit of information. Unlike the std::bitset
   * class, the size of the BitSet can be specified (and modified) at runtime.
   *
   * This class is useful for storing a single boolean value (true or false)
   * for each element of some vector. It may also be useful for two-dimensional
   * data, though the programmer is responsible for performing the conversion
   * from a two-dimensional position to an array index. If a boolean value for
   * each voxel is required for three- or four-dimensional data, use of an
   * Image::BufferScratch<bool> is recommended. */
  class BitSet { NOMEMALIGN

    public:

      /*! convenience classes that allow the programmer to access and modify
       * the bit wise information using the [] operator. */
      class Value { NOMEMALIGN
        public:
        Value (BitSet& master, const size_t offset) : d (master), p (offset) { assert (p < d.size()); }
        operator bool()                  const { return d.test (p); }
        bool   operator== (const bool i) const { return (i == d.test (p)); }
        bool   operator=  (const bool i)       { i ? d.set (p) : d.reset (p); return i; }
        Value& operator|= (const bool i)       { (i || bool(*this)) ? d.set (p) : d.reset (p); return *this; }
        Value& operator&= (const bool i)       { (i && bool(*this)) ? d.set (p) : d.reset (p); return *this; }
        friend std::ostream& operator<< (std::ostream& stream, const Value& V) { stream << (bool(V) ? '1' : '0'); return stream; }
        private:
        BitSet& d;
        const size_t p;
      };
      class ConstValue { NOMEMALIGN
        public:
          ConstValue (const BitSet& master, const size_t offset) : d (master), p (offset) { assert (p < d.size()); }
          operator bool()                  const { return d.test (p); }
          bool   operator== (const bool i) const { return (i == d.test (p)); }
          friend std::ostream& operator<< (std::ostream& stream, const ConstValue& V) { stream << (bool(V) ? '1' : '0'); return stream; }
        private:
          const BitSet& d;
          const size_t p;
      };


      //! create a new bitset with the desired size.
      /*! If \a allocator is unspecified or set to false, the initial
       * value for each element will be false. If it is specified as true,
       * then all data will be initialised as true. */
      BitSet (const size_t, const bool allocator = false);

      //! copy-construct a bitset, with explicit copying of data into the new instance
      /*! The BitSet copy-constructor explicitly copies all of the data from the
       * constructing instance into the new instance. Therefore, their sizes and data
       * will be identical, but subsequent modifications to the data in one instance
       * will not affect the other. */
      BitSet (const BitSet&);
      ~BitSet();

      //! resize the bitset, retaining existing data
      /*! Modify the size of the BitSet. Existing data information will be retained
       * by the resizing process. If the new size is smaller than the existing size,
       * then all excess data will be truncated. If the new size is larger than the
       * existing size, then all existing data will be retained, and additional
       * bits beyond the previous size will be set to false; unless \a allocator is
       * explicitly provided as true, in which case all additional bits will be set
       * to true. */
      void resize (const size_t, const bool allocator = false);

      //! clear the data
      /*! Clears all existing data in the BitSet. By default all values will be set
       * to false; if \a allocator is explicitly set to true, then all values will
       * be set to true. */
      void clear  (const bool allocator = false);

      //! access boolean value at a given index
      /*! These functions provide access to the raw boolean data using the []
       * (square-bracket) operator. Both const and non-const versions are provided.
       * Although internally the BitSet class stores eight boolean values in each
       * byte of memory (to minimise memory usage), these operators can be used to
       * access and manipulate the bit wise data without corrupting the surrounding
       * data.
       * \returns a Value or ConstValue class used to manipulate the bit data at
       * the specified index */
      ConstValue operator[] (const size_t i) const { return ConstValue (*this, i); }
      Value      operator[] (const size_t i)       { return Value      (*this, i); }

      //! the number of boolean elements in the set
      /*! The size of the BitSet. Note that this is the number of boolean values
       * stored in the array; NOT the memory consumption of the class.
       * \returns the number of boolean elements in the BitSet. */
      size_t size() const { return bits; }

      //! whether or not the bitset is 'full' i.e. all elements are true
      /*! Convenience function for testing whether or not the BitSet is 'full',
       * i.e. all elements in the array are set to true. This can be useful if
       * the programmer chooses not to manually keep track of the number of
       * entries set or not set. Because it processes the data in bytes rather
       * than bits, it is faster than the programmer manually performing this
       * calculation.
       * \returns true if all elements are set to true, false otherwise. */
      bool full()  const;

      //! whether or not the bitset is 'empty' i.e. all elements are false
      /*! Convenience function for testing whether or not the BitSet is 'empty',
       * i.e. all elements in the array are set to false. This can be useful if
       * the programmer chooses not to manually keep track of the number of
       * entries set or not set. Because it processes the data in bytes rather
       * than bits, it is faster than the programmer manually performing this
       * calculation.
       * \returns true if all elements are set to false, false otherwise. */
      bool empty() const;

      //! count the number of true entries in the set
      /*! Convenience function for counting the number of true entries in the
       * set. This can be useful if the programmer chooses not to manually keep
       * track of the number of entries set or not set. The number of entries
       * in the data that are set to false can be calculated as:
       * \code
       * BitSet data (1000);
       * // ...
       * const size_t false_count = data.size() - data.count();
       * \endcode
       * \returns the number of elements in the array set to true. */
      size_t count() const;

      //! convenience functions for performing boolean operations
      /*! Convenience function for performing boolean operations using BitSet
       * data. Each of these functions performs a particular boolean operation,
       * but for all of the data in the array. Because they process the data
       * in bytes rather than bits, they are faster than if the programmer
       * manually performed these operations on a per-bit basis.
       *
       * Particular notes of interest:
       * - The '=' (assignment) operator will copy both the size of the
       * passed BitSet, and the data itself.
       * - The '==' (comparison) operator will return false if the two BitSets
       * differ in their number of bits. If the programmer wishes to compare
       * two BitSets of different sizes, where only the length of the smaller
       * BitSet is considered, this can be achieved as follows:
       * \code
       * BitSet A (1000), B (2000);
       * // ...
       * BitSet B_small (B);
       * B_small.resize (A.size());
       * if (A == B_small) {
       *   // Do something
       * }
       * \endcode
       * */
      BitSet& operator=  (const BitSet&);
      bool    operator== (const BitSet&) const;
      bool    operator!= (const BitSet&) const;
      BitSet& operator|= (const BitSet&);
      BitSet& operator&= (const BitSet&);
      BitSet& operator^= (const BitSet&);
      BitSet  operator|  (const BitSet&) const;
      BitSet  operator&  (const BitSet&) const;
      BitSet  operator^  (const BitSet&) const;
      BitSet  operator~  () const;

      const uint8_t* get_data_ptr() const { return data; }

      friend std::ostream& operator<< (std::ostream&, BitSet&);


    protected:
      size_t   bits;
      size_t   bytes;

      size_t excess_bits() const { return (bits - (8 * (bytes - 1))); }

      bool test  (const size_t index) const
      {
        assert (index < bits);
        return (data[index>>3] & masks[index&7]);
      }

      void set   (const size_t index)
      {
        assert (index < bits);
        std::atomic<uint8_t>* at = reinterpret_cast<std::atomic<uint8_t>*> (((uint8_t*) data) + (index>>3));
        uint8_t prev = *at, new_value;
        do { new_value = prev | masks[index&7]; } while (!at->compare_exchange_weak (prev, new_value));
      }

      void reset (const size_t index)
      {
        assert (index < bits);
        std::atomic<uint8_t>* at = reinterpret_cast<std::atomic<uint8_t>*> (((uint8_t*) data) + (index>>3));
        uint8_t prev = *at, new_value;
        do { new_value = prev & ~masks[index&7]; } while (!at->compare_exchange_weak (prev, new_value));
      }


    private:
      uint8_t* data;

      static const uint8_t masks[8];
      static const char dbyte_to_hex[16];

      std::string byte_to_hex (const uint8_t d) const
      {
        std::string out;
        for (size_t i = 0; i != 2; ++i) {
          const uint8_t dm = i ? (d & 0x0F) : ((d & 0xF0) >> 4);
          out.push_back (dbyte_to_hex[dm]);
        }
        return out;
      }



  };



}

#endif
