/*
   Copyright 2008 Brain Research Institute, Melbourne, Australia

   Written by Robert Smith, 2010.

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


#ifndef __dwi_directions_mask_h__
#define __dwi_directions_mask_h__


#include <fstream>


#include "dwi/directions/set.h"



namespace MR {
  namespace DWI {
    namespace Directions {



      class Mask {

        private:
          class Value {
            public:
              Value (Mask& master, size_t pos) : d (master), p (pos) { }
              operator bool () const { return d.test (p); }
              bool   operator=  (const bool i) { i ? d.set (p) : d.reset (p); return i; }
              Value& operator|= (const bool i) { if (i || d.test (p)) d.set (p);    else d.reset (p); return *this; }
              Value& operator&= (const bool i) { if (i && d.test (p)) return *this; else d.reset (p); return *this; }
              friend std::ostream& operator<< (std::ostream& stream, const Value& V) { stream << (bool(V) ? '1' : '0'); return stream; }
            private:
              Mask& d;
              const size_t p;
          };


        public:
          Mask (const Set& master, const bool allocator = false) :
            dirs (master),
            data (new uint8_t [bytes()]) {
              memset (data, (allocator ? 0xFF : 0x00), bytes());
            }

          Mask (const Mask& that) :
            dirs (that.dirs),
            data (new uint8_t [bytes()]) {
              memcpy (data, that.data, bytes());
            }

          ~Mask() {
            delete [] data;
          }


          const Set& get_dirs() const { return dirs; }

          void clear (const bool allocator = false) {
            memset (data, (allocator ? 0xFF : 0x00), bytes());
          }

          bool full()  const;
          bool empty() const;

          void erode  (const size_t iterations = 1);
          void dilate (const size_t iterations = 1);

          bool is_adjacent (const size_t) const;
          size_t get_min_linkage (const Mask&);

          size_t count (const bool value = true) const;

          Value operator[] (size_t index)       { return Value (*this, index); }
          bool  operator[] (size_t index) const { return test (index); }

          Mask& operator= (const Mask& that) {
            assert (that.size() == size());
            memcpy (data, that.data, bytes());
            return *this;
          }

          bool operator== (const Mask&) const;
          bool operator!= (const Mask& that) const {
            return (!(*this == that));
          }
          Mask& operator|= (const Mask&);
          Mask& operator&= (const Mask&);
          Mask& operator^= (const Mask&);

          Mask operator| (const Mask& that) const {
            Mask result (*this);
            result |= that;
            return result;
          }

          Mask operator& (const Mask& that) const {
            Mask result (*this);
            result &= that;
            return result;
          }

          Mask operator^ (const Mask& that) const {
            Mask result (*this);
            result ^= that;
            return result;
          }

          Mask  operator~  () const;

          friend std::ostream& operator<< (std::ostream& stream, Mask& d);

          size_t size() const { return dirs.size(); }


        private:
          const Set& dirs;
          uint8_t* data;

          bool test  (const size_t index) const { return (data[index>>3] &   masks[index&7]); }
          void set   (const size_t index)       {         data[index>>3] |=  masks[index&7] ; }
          void reset (const size_t index)       {         data[index>>3] &= ~masks[index&7] ; }

          size_t  bytes()       const { return dirs.dir_mask_bytes; }
          size_t  excess_bits() const { return dirs.dir_mask_excess_bits; }
          uint8_t excess_mask() const { return dirs.dir_mask_excess_bits_mask; }

          std::string byte_to_hex (const uint8_t) const;

          static const uint8_t masks[8];

      };


    }
  }
}

#endif
