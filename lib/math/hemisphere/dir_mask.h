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


#ifndef __math_hemisphere_dir_mask_h__
#define __math_hemisphere_dir_mask_h__



#include "math/hemisphere/directions.h"



namespace MR {
namespace Math {
namespace Hemisphere {



class Dir_mask {

  private:
    class Value {
      public:
        Value (Dir_mask& master, size_t pos) : d (master), p (pos) { }
        operator bool () const { return d.test (p); }
        bool   operator=  (const bool i) { i ? d.set (p) : d.reset (p); return i; }
        Value& operator|= (const bool i) { if (i || d.test (p)) d.set (p);    else d.reset (p); return *this; }
        Value& operator&= (const bool i) { if (i && d.test (p)) return *this; else d.reset (p); return *this; }
        friend std::ostream& operator<< (std::ostream& stream, const Value& V) { stream << (bool(V) ? '1' : '0'); return stream; }
      private:
        Dir_mask& d;
        const size_t p;
    };


  public:

    Dir_mask (const Directions&, const bool allocator = false);
    Dir_mask (const Dir_mask&);
    ~Dir_mask();

    const Directions& get_dirs() const { return dirs; }

    void clear (const bool allocator = false);

    bool full()  const;
    bool empty() const;

    void erode  (const size_t iterations = 1);
    void dilate (const size_t iterations = 1);

    bool is_adjacent (const size_t) const;
    size_t get_min_linkage (const Dir_mask&);

    size_t count (const bool value = true) const;

    Value operator[] (size_t index)       { return Value (*this, index); }
    bool  operator[] (size_t index) const { return test (index); }

    Dir_mask& operator=  (const Dir_mask&);
    bool      operator== (const Dir_mask&) const;
    bool      operator!= (const Dir_mask&) const;
    Dir_mask& operator|= (const Dir_mask&);
    Dir_mask& operator&= (const Dir_mask&);
    Dir_mask& operator^= (const Dir_mask&);
    Dir_mask  operator|  (const Dir_mask&) const;
    Dir_mask  operator&  (const Dir_mask&) const;
    Dir_mask  operator^  (const Dir_mask&) const;
    Dir_mask  operator~  () const;

    friend std::ofstream& operator<< (std::ofstream& stream, Dir_mask& d);

    size_t size() const { return dirs.get_num_dirs(); }


  private:
    const Directions& dirs;
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
