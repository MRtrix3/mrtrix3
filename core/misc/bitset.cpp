/* Copyright (c) 2008-2023 the MRtrix3 contributors.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Covered Software is provided under this License on an "as is"
 * basis, without warranty of any kind, either expressed, implied, or
 * statutory, including, without limitation, warranties that the
 * Covered Software is free of defects, merchantable, fit for a
 * particular purpose or non-infringing.
 * See the Mozilla Public License v. 2.0 for more details.
 *
 * For more details, see http://www.mrtrix.org/.
 */

#include "misc/bitset.h"



namespace MR {



  const uint8_t BitSet::masks[8] = {0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80};



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
        if (have_excess_bits())
          new_data[bytes - 1] = allocator ?
                                (data[bytes - 1] | excess_bit_mask()) :
                                (data[bytes - 1] & ~excess_bit_mask());
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
    const size_t bytes_to_test = have_excess_bits() ? bytes - 1 : bytes;
    for (size_t i = 0; i != bytes_to_test; ++i) {
      if (data[i] != 0xFF)
        return false;
    }
    if (!have_excess_bits())
      return true;
    if ((data[bytes - 1] | excess_bit_mask()) != 0xFF)
      return false;
    return true;
  }



  bool BitSet::empty() const
  {
    const size_t bytes_to_test = have_excess_bits() ? bytes - 1 : bytes;
    for (size_t i = 0; i != bytes_to_test; ++i) {
      if (data[i])
        return false;
    }
    if (!have_excess_bits())
      return true;
    if (data[bytes - 1] & ~excess_bit_mask())
      return false;
    return true;
  }



  size_t BitSet::count () const
  {
    static const uint8_t byte_to_count[256] = /*0x00-0x0F*/ { 0, 1, 1, 2, 1, 2, 2, 3, 1, 2, 2, 3, 2, 3, 3, 4,
                                              /*0x10-0x1F*/   1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5,
                                              /*0x20-0x2F*/   1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5,
                                              /*0x30-0x3F*/   2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
                                              /*0x40-0x3F*/   1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5,
                                              /*0x50-0x5F*/   2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
                                              /*0x60-0x6F*/   2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
                                              /*0x70-0x7F*/   3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7,
                                              /*0x80-0x8F*/   1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5,
                                              /*0x90-0x9F*/   2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
                                              /*0xA0-0xAF*/   2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
                                              /*0xB0-0xBF*/   3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7,
                                              /*0xC0-0xCF*/   2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
                                              /*0xD0-0xDF*/   3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7,
                                              /*0xE0-0xEF*/   3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7,
                                              /*0xF0-0xFF*/   4, 5, 5, 6, 5, 6, 6, 7, 5, 6, 6, 7, 6, 7, 7, 8 };
    size_t count = 0;
    const size_t bytes_to_test = have_excess_bits() ? bytes - 1 : bytes;
    for (size_t i = 0; i != bytes_to_test; ++i)
      count += byte_to_count[data[i]];
    for (size_t i = 8 * bytes_to_test; i != bits; ++i) {
      if ((*this)[i])
        ++count;
    }
    return count;
  }






  std::ostream& operator<< (std::ostream& stream, const BitSet& d)
  {
    if (!d.bytes)
      return stream;

    static const char dbyte_to_hex[16] = {'0','1','2','3','4','5','6','7','8','9','A','B','C','D','E','F'};

    auto byte_to_hex = [] (const uint8_t d)
    {
      std::string out;
      for (size_t i = 0; i != 2; ++i) {
        const uint8_t dm = i ? (d & 0x0F) : ((d & 0xF0) >> 4);
        out.push_back (dbyte_to_hex[dm]);
      }
      return out;
    };

    stream << "0x";
    if (d.have_excess_bits()) {
      stream << byte_to_hex (d.data[d.bytes - 1] & (0xFF >> d.excess_bits()));
      for (ssize_t i = d.bytes - 2; i >= 0; --i)
        stream << byte_to_hex (d.data[i]);
    } else {
      for (ssize_t i = d.bytes - 1; i >= 0; --i)
        stream << byte_to_hex (d.data[i]);
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
    if (have_excess_bits()) {
      if (memcmp (data, that.data, bytes - 1))
        return false;
      return ((data[bytes - 1] & ~excess_bit_mask()) == (that.data[bytes - 1] & ~excess_bit_mask()));
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
