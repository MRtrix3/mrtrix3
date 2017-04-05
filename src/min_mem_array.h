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




#ifndef __min_mem_array_h__
#define __min_mem_array_h__

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstdint>


namespace MR {



// This class allows for construction and management of a c-style array, where the
//   memory overhead is minimal (a pointer and a size_t)
// std::vector<>'s have some amount of overhead, which can add up if many are being stored
// Typical usage is to gather the required data using a std::vector<>, and use that vector
//   to construct a Min_mem_array<>

template <class T>
class Min_mem_array {

  public:
    Min_mem_array () :
      n (0),
      d (NULL) { }

    Min_mem_array (const T& i) :
      n (1),
      d (new T[1])
    {
      d[0] = i;
    }

    Min_mem_array (const size_t size, const T& i) :
      n (size),
      d (new T[n])
    {
      for (size_t a = 0; a != n; ++a)
        d[n] = i;
    }

    template <class C>
    Min_mem_array (const C& data) :
      n (data.size()),
      d (new T[data.size()])
    {
      size_t index = 0;
      for (typename C::const_iterator i = data.begin(); i != data.end(); ++i, ++index)
        d[index] = *i;
    }

    Min_mem_array (const Min_mem_array& that) :
      n (that.n),
      d (new T[n])
    {
      memcpy (d, that.d, n * sizeof (T));
    }

    virtual ~Min_mem_array()
    {
      delete[] d;
      d = NULL;
    }

    void add (const T& i) {
      if (d) {
        T* new_data = new T[n + 1];
        memcpy (new_data, d, n * sizeof (T));
        new_data[n] = i;
        delete[] d;
        d = new_data;
        ++n;
      } else {
        d = new T[1];
        d[0] = i;
        n = 1;
      }
    }

    // Second version of add() which invokes copy constructors in the underlying data
    void add_copyconstruct (const T& i) {
      if (d) {
        T* new_data = new T[n + 1];
        for (size_t a = 0; a != n; ++a)
          new_data[a] = d[a];
        new_data[n] = i;
        delete[] d;
        d = new_data;
        ++n;
      } else {
        d = new T[1];
        d[0] = i;
        n = 1;
      }
    }


    void erase() {
      if (d) {
        delete[] d;
        d = NULL;
        n = 0;
      }
    }

    // TODO Should be 2 versions of this; 1 retains current size, other takes on size of container
    // Need to make sure change does not affect any existing code
    template <class C>
    void load (C& data) const {
      for (size_t i = 0; i != n; ++i)
        data.push_back (d[i]);
    }

    size_t dim() const { return n; }

    bool operator== (const Min_mem_array<T>& that) const {
      return ((n == that.n) && !memcmp (d, that.d, n * sizeof (T)));
    }

    bool operator< (const Min_mem_array<T>& that) const {
      // If one is empty and the other is not, one is 'less than' the other; but if both are empty, then '<' should return false
      if (!n)
        return that.n;
      if (!that.n)
        return false;
      for (size_t i = 0; i != std::min (n, that.n); ++i) {
        if (d[i] < that.d[i])
          return true;
        if (d[i] > that.d[i])
          return false;
        // Continue to the next element if they are identical
      }
      // If code reaches this point, then all of the elements up to the minimum of the two lengths are identical
      // Therefore, only return true for '<' if the length of that is longer than this (if they are equivalent in length, '<' does not hold so return false)
      return (n < that.n);
    }

    T& operator[] (const size_t i) {
      assert (i < n);
      return d[i];
    }

    const T& operator[] (const size_t i) const {
      assert (i < n);
      return d[i];
    }

    Min_mem_array<T>& operator= (const Min_mem_array<T>& that) {
      n = that.n;
      if (d)
        delete[] d;
      d = new T[n];
      memcpy (d, that.d, n * sizeof (T));
      return (*this);
    }


  private:
    size_t n;
    T* d;

};


}

#endif
