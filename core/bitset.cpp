/*
 * Copyright (c) 2008-2018 the MRtrix3 contributors.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, you can obtain one at http://mozilla.org/MPL/2.0/
 *
 * MRtrix3 is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * For more details, see http://www.mrtrix.org/
 */


#include "bitset.h"



namespace MR {



  const uint8_t BitSet::masks[8] = {0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80};

  const char BitSet::dbyte_to_hex[16] = {'0','1','2','3','4','5','6','7','8','9','A','B','C','D','E','F'};


  BitSet::BitSet (const size_t b, const bool allocator) :
      bits  (b),
      bytes ((bits + 7) / 8),
      data  (new uint8_t[bytes])
  {
    memset (data, (allocator ? 0xFF : 0x00), bytes);
  }


  BitSet::BitSet (const BitSet& that) :
      bits  (that.bits),
      bytes (that.bytes),
      data  (new uint8_t[bytes])
  {
    memcpy (data, that.data, bytes);
  }


  BitSet::~BitSet() {
    delete[] data; data = nullptr;
  }





  void BitSet::resize (const size_t new_size, const bool allocator)
  {

    size_t new_bits;
    new_bits = new_size;
    const size_t new_bytes = (new_bits + 7) / 8;
    uint8_t* new_data = new uint8_t[new_bytes];
    if (bytes) {
      if (new_bytes > bytes) {
        memcpy (new_data, data, bytes);
        memset (new_data + bytes, (allocator ? 0xFF : 0x00), new_bytes - bytes);
        const uint8_t mask = 0xFF << excess_bits();
        data[bytes - 1] = allocator ? (data[bytes - 1] | mask) : (data[bytes - 1] & ~mask);
      } else {
        memcpy (new_data, data, new_bytes);
      }
    } else {
      memset (new_data, (allocator ? 0xFF : 0x00), new_bytes);
    }
    delete[] data;
    bits = new_bits;
    bytes = new_bytes;
    data = new_data;
    new_data = nullptr;

  }


  void BitSet::clear (const bool allocator)
  {
    memset(data, (allocator ? 0xFF : 0x00), bytes);
  }


  bool BitSet::full() const
  {

    const size_t bytes_to_test = (bits % 8) ? bytes - 1 : bytes;
    for (size_t i = 0; i != bytes_to_test; ++i) {
      if (data[i] != 0xFF)
        return false;
    }
    if (!(bits % 8))
      return true;

    const uint8_t mask = 0xFF << excess_bits();
    if ((data[bytes - 1] | mask) != 0xFF)
      return false;
    return true;

  }


  bool BitSet::empty() const
  {

    const size_t bytes_to_test = (bits % 8) ? bytes - 1 : bytes;
    for (size_t i = 0; i != bytes_to_test; ++i) {
      if (data[i])
        return false;
    }
    if (!(bits % 8))
      return true;

    const size_t excess_bits = bits - (8 * (bytes - 1));
    const uint8_t mask = ~(0xFF << excess_bits);
    if (data[bytes - 1] & mask)
      return false;
    return true;

  }


  size_t BitSet::count () const
  {
    size_t count = 0;
    for (size_t i = 0; i != bits; ++i) {
      if (test (i))
        ++count;
    }
    return count;
  }





  std::ostream& operator<< (std::ostream& stream, BitSet& d)
  {
    stream << "0x";
    if (d.excess_bits()) {
      const uint8_t mask = 0xFF << d.excess_bits();
      stream << d.byte_to_hex (d.data[d.bytes - 1] & mask);
      for (size_t i = d.bytes - 2; i--;)
        stream << d.byte_to_hex (d.data[i]);
    } else {
      for (size_t i = d.bytes - 1; i--;)
        stream << d.byte_to_hex (d.data[i]);
    }
    return stream;
  }







  BitSet& BitSet::operator= (const BitSet& that)
  {
    delete[] data;
    bits = that.bits;
    bytes = that.bytes;
    data = new uint8_t[bytes];
    memcpy (data, that.data, bytes);
    return *this;
  }


  bool BitSet::operator== (const BitSet& that) const
  {
    if (bits != that.bits)
      return false;
    if (bits % bytes) {
      if (memcmp(data, that.data, bytes - 1))
        return false;
      const uint8_t mask = ~(0xFF << excess_bits());
      if ((data[bytes - 1] & mask) != (that.data[bytes - 1] & mask))
        return false;
      return true;
    } else {
      return (!memcmp (data, that.data, bytes));
    }
  }


  bool BitSet::operator!= (const BitSet& that) const
  {
    return (!(*this == that));
  }


  BitSet& BitSet::operator|= (const BitSet& that)
  {
    assert (bits == that.bits);
    for (size_t i = 0; i != bytes; ++i)
      data[i] |= that.data[i];
    return *this;
  }


  BitSet& BitSet::operator&= (const BitSet& that)
  {
    assert (bits == that.bits);
    for (size_t i = 0; i != bytes; ++i)
      data[i] &= that.data[i];
    return *this;
  }


  BitSet& BitSet::operator^= (const BitSet& that)
  {
    assert (bits == that.bits);
    for (size_t i = 0; i != bytes; ++i)
      data[i] ^= that.data[i];
    return *this;
  }


  BitSet BitSet::operator| (const BitSet& that) const
  {
    BitSet result (*this);
    result |= that;
    return result;
  }


  BitSet BitSet::operator& (const BitSet& that) const
  {
    BitSet result (*this);
    result &= that;
    return result;
  }


  BitSet BitSet::operator^ (const BitSet& that) const
  {
    BitSet result (*this);
    result ^= that;
    return result;
  }


  BitSet BitSet::operator~() const
  {
    BitSet result (*this);
    for (size_t i = 0; i != bytes; ++i)
      result.data[i] = ~data[i];
    return result;
  }






}
