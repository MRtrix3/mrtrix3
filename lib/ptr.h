/*
    Copyright 2008 Brain Research Institute, Melbourne, Australia

    Written by J-Donald Tournier, 27/06/08.

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

#ifndef __ptr_h__
#define __ptr_h__

#include <vector>
#include "mrtrix.h"

/** \defgroup Memory Memory management
 * \brief Classes & functions to ease memory management. */

namespace MR
{

  /** \addtogroup Memory
   * @{ */





  //! A managed vector of pointers
  /*! This class holds a dynamic array of pointers to objects and perform
   * allocation and deallocation of the objects pointed to as needed.
   * For example:
   * \code
   * void my_function () {
   *   VecPtr<Object> array (10);   // an array of 10 NULL pointers to Object
   *
   *   array[0] = new Object;
   *   array[1] = new Object (*array[0]);
   *   array[0] = NULL;             // deletes previous object at that location
   *
   *   array[8] = new Object;
   *   array.resize (5);            // deletes all objects above index 5
   *
   *   if (error) throw Exception ("oops"); // all pointers are freed in destructor
   *   ...
   * } // all pointers are freed in destructor
   * \endcode
   *
   * Usage is mostly identical to that of std::pointer, although with a limited
   * feature set.
   *
   * \note As for the Ptr<T> class, use the \a is_array template parameter when
   * handling arrays of objects. This ensures the array version of the \c
   * delete operator is appropriately called when the array is freed.
   *
   * \sa Ptr A simple smart pointer
   */
  template <class T, bool is_array = false> class VecPtr
  {
    public:
      typedef typename std::vector<T*>::iterator iterator;
      typedef typename std::vector<T*>::const_iterator const_iterator;
      typedef typename std::vector<T*>::reference reference;
      typedef typename std::vector<T*>::const_reference const_reference;

      VecPtr () { }
      //! create a list of NULL pointers of size \a num.
      VecPtr (size_t num) : V (num) {
        for (size_t i = 0; i < size(); ++i)
          V[i] = NULL;
      }
      //! destructor: all non-NULL pointers will be freed
      ~VecPtr() {
        for (size_t i = 0; i < size(); ++i)
          dealloc (V[i]);
      }

      size_t size () const {
        return V.size();
      }
      bool empty () const {
        return V.empty();
      }
      reference operator[] (size_t i)       {
        return V[i];
      }
      const_reference operator[] (size_t i) const {
        return V[i];
      }

      void resize (size_t new_size) {
        size_t old_size = size();
        for (size_t i = new_size; i < old_size; ++i)
          dealloc (V[i]);
        V.resize (new_size);
        for (size_t i = old_size; i < new_size; ++i)
          V[i] = NULL;
      }

      void clear () {
        for (size_t i = 0; i < size(); ++i)
          dealloc (V[i]);
        V.clear();
      }

      iterator begin() {
        return V.begin();
      }
      const_iterator begin() const {
        return V.begin();
      }
      iterator end() {
        return V.end();
      }
      const_iterator end() const {
        return V.end();
      }

      reference front () {
        return V.front();
      }
      const_reference front () const {
        return V.front();
      }
      reference back () {
        return V.back();
      }
      const_reference back () const {
        return V.back();
      }

      void push_back (T* item) {
        V.push_back (item);
      }
      void pop_back () {
        dealloc (V.back());
        V.pop_back();
      }

      iterator insert (iterator position, T* item = NULL) {
        iterator ret = V.insert (position, NULL);
        *position = item;
        return ret;
      }
      void insert (iterator position, size_t n) {
        V.insert (position, n, NULL);
      }

      T* release (size_t i) {
        T* ret = V[i];
        V[i] = NULL;
        return ret;
      }

      iterator erase (iterator position) {
        dealloc (*position);
        return V.erase (position);
      }
      iterator erase (iterator first, iterator last) {
        for (iterator it = first; it != last; ++it)
          dealloc (*it);
        return V.erase (first, last);
      }

    private:
      std::vector<T*> V;
      void dealloc (T* p) {
        if (is_array)
          delete [] p;
        else
          delete p;
      }
  };



  /** @} */
}

#endif


