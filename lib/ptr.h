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



  //! A simple smart pointer implementation
  /*! A simple pointer class that will delete the object it points to when it
   * goes out of scope. This is useful to ensure that allocated objects are
   * destroyed without having to explicitly delete them before every possible
   * exit point. It behaves in almost all other respects like a standard
   * pointer. For example:
   * \code
   * void my_function () {
   *   Ptr<MyObject> object (new Object);
   *
   *   object->member = something;       // member-by-pointer operator
   *   call_by_reference (*object);      // dereference operator
   *   call_by_pointer (object.get());   // get actual address of object
   *   object = new Object (parameters); // delete previous object and point to new one
   *
   *   if (something) return; // object goes out of scope: pointer is freed in destructor
   *
   *   ...
   *   // do something useful
   *   ...
   *   if (error) throw Exception ("oops"); // pointer is freed in destructor
   *   ...
   * } // object goes out of scope: pointer is freed in destructor
   * \endcode
   *
   * \note Instances of Ptr<T> cannot be copied, since there would then be
   * ambiguity as to which instance is responsible for deleting the object.
   * For this reason, both the copy constructor and the standard assignment
   * operator are declared private. This also implies that this class cannot be
   * used in STL containers, such as std::vector.
   *
   * \sa RefPtr A reference-counting pointer
   * \sa VecPtr A %vector of pointers %to objects
   */
  template <class T, bool is_array = false> class Ptr
  {
    public:
      explicit Ptr (T* p = NULL) throw () : ptr (p) { }
      ~Ptr () {
        dealloc();
      }

      //! test whether the pointer is non-NULL
      operator bool() const throw ()       {
        return (ptr);
      }
      //! test whether the pointer is NULL
      bool     operator! () const throw () {
        return (!ptr);
      }

      //! Assignment operator
      /*! This will free the object currently pointed to by the Ptr<T> (if
       * any), and manage the new object instead. */
      Ptr&    operator= (T* p)   {
        dealloc();
        ptr = p;
        return (*this);
      }
      bool    operator== (const Ptr& R) const throw () {
        return (ptr == R.ptr);
      }
      bool    operator!= (const Ptr& R) const throw () {
        return (ptr != R.ptr);
      }
      T&      operator*() const throw ()   {
        return (*ptr);
      }
      T&      operator[] (size_t n) const throw ()   {
        return (ptr[n]);
      }
      T*      operator->() const throw ()  {
        return (ptr);
      }
      //! Return address of actual object
      T*      get () const throw ()        {
        return (ptr);
      }
      //! Return address of actual object, and stop managing the object.
      /*! Use this function to stop the Ptr<T> class from managing the object.
       * The destructor will NOT call delete on the object when it goes out of
       * scope: the user is responsible for ensuring that the object is freed
       * via other means. */
      T* release () const throw ()    {
        T* p (ptr);
        ptr = NULL;
        return (p);
      }

      //! comparison operator: calls the object's own comparison operator
      bool    operator< (const Ptr& R) const {
        return (*ptr < * (R.ptr));
      }

    private:
      T*   ptr;
      void dealloc () {
        if (is_array) delete [] ptr;
        else delete ptr;
      }

      Ptr (const Ptr& R) : ptr (NULL) {
        assert (0);
      };
      Ptr& operator= (const Ptr& R) {
        assert (0);
        return (*this);
      }
  };





  template <class T, bool is_array = false> class RefPtr
  {
    public:
      explicit RefPtr (T* p = NULL) throw () : ptr (p), count (new size_t) {
        *count = 1;
      }
      RefPtr (const RefPtr& R) throw () : ptr (R.ptr), count (R.count) {
        ++*count;
      }
      ~RefPtr () {
        if (*count == 1) {
          dealloc();
          delete count;
        }
        else --*count;
      }

      operator bool() const throw ()       {
        return (ptr);
      }
      bool     operator! () const throw () {
        return (!ptr);
      }

      RefPtr&    operator= (const RefPtr& R) {
        if (this == &R) return (*this);
        if (*count == 1) {
          dealloc();
          delete count;
        }
        else --*count;
        ptr = R.ptr;
        count = R.count;
        ++*count;
        return (*this);
      }
      RefPtr&    operator= (T* p) {
        if (ptr == p) return (*this);
        if (*count == 1) dealloc();
        else {
          --*count;
          count = new size_t;
          *count = 1;
        }
        ptr = p;
        return (*this);
      }

      bool    operator== (const RefPtr& R) const throw () {
        return (ptr == R.ptr);
      }
      bool    operator!= (const RefPtr& R) const throw () {
        return (ptr != R.ptr);
      }

      T&      operator*() const throw ()   {
        return (*ptr);
      }
      T*      operator->() const throw ()  {
        return (ptr);
      }
      T*      get() const throw ()         {
        return (ptr);
      }
      bool    unique() const throw ()      {
        return (*count == 1);
      }

      bool    operator< (const RefPtr& R) const {
        return (*ptr < *R.ptr);
      }

      template <class U, bool> friend class RefPtr;
      template <class U> RefPtr (const RefPtr<U,is_array>& R) throw () : ptr (R.ptr), count (R.count) {
        ++*count;
      }
      template <class U> RefPtr<U,is_array>& operator= (const RefPtr<U,is_array>& R) {
        if (this == &R) return (*this);
        if (*count == 1) {
          dealloc();
          delete count;
        }
        else --*count;
        ptr = R.ptr;
        count = R.count;
        ++*count;
        return (*this) ;
      }

      friend std::ostream& operator<< (std::ostream& stream, const RefPtr<T,is_array>& R) {
        stream << "(" << R.ptr << "): ";
        if (R) stream << *R.ptr;
        else stream << "null";
        stream << " (" << *R.count << " refs, counter at " << R.count << ")";
        return (stream);
      }

    private:
      T*      ptr;
      size_t* count;

      void dealloc () {
        if (is_array) delete [] ptr;
        else delete ptr;
      }
  };


  template <class T, bool is_array = false> class VecPtr
  {
    public:
      typedef typename std::vector<T*>::iterator iterator;
      typedef typename std::vector<T*>::const_iterator const_iterator;
      typedef typename std::vector<T*>::reference reference;
      typedef typename std::vector<T*>::const_reference const_reference;

      VecPtr () { }
      VecPtr (size_t num) : V (num) {
        for (size_t i = 0; i < size(); ++i) V[i] = NULL;
      }
      ~VecPtr() {
        for (size_t i = 0; i < size(); ++i) dealloc (V[i]);
      }

      size_t size () const {
        return (V.size());
      }
      bool empty () const {
        return (V.empty());
      }
      reference operator[] (size_t i)       {
        return (V[i]);
      }
      const_reference operator[] (size_t i) const {
        return (V[i]);
      }

      void resize (size_t new_size) {
        size_t old_size = size();
        for (size_t i = new_size; i < old_size; ++i) dealloc (V[i]);
        V.resize (new_size);
        for (size_t i = old_size; i < new_size; ++i) V[i] = NULL;
      }

      iterator       begin()        {
        return (V.begin());
      }
      const_iterator begin() const  {
        return (V.begin());
      }
      iterator       end()          {
        return (V.end());
      }
      const_iterator end() const    {
        return (V.end());
      }

      reference       front ()       {
        return (V.front());
      }
      const_reference front () const {
        return (V.front());
      }
      reference       back ()        {
        return (V.back());
      }
      const_reference back () const  {
        return (V.back());
      }

      void push_back (T* item) {
        V.push_back (item);
      }
      void pop_back () {
        dealloc (V.back());
        V.pop_back();
      }

      iterator insert (iterator position, T* item = NULL) {
        V.insert (position, NULL);
        *position = item;
      }
      void insert (iterator position, size_t n) {
        V.insert (position, n, NULL);
      }

      T* release (size_t i) {
        T* ret = V[i];
        V[i] = NULL;
        return (ret);
      }

      iterator erase (iterator position) {
        dealloc (*position);
        return (V.erase (position));
      }
      iterator erase (iterator first, iterator last) {
        for (iterator it = first; it != last; ++it) dealloc (*it);
        return (V.erase (first, last));
      }

    private:
      std::vector<T*> V;
      void dealloc (T* p) {
        if (is_array) delete [] p;
        else delete p;
      }
  };






  /** @} */
}

#endif


