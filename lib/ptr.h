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

  //! a function to compare two pointers based on the objects they reference
  /*! This function is useful to pass to function such as std::sort(), for
  * example. */
  class PtrComp {
    public:
      template <class A, class B> bool operator() (const A& a, const B& b) const {
        return *a < *b;
      }
  };



  //! A simple smart pointer implementation
  /*! A simple pointer class that will delete the object it points to when it
   * is destroyed. This is useful to ensure that allocated objects are
   * destroyed without having to explicitly delete them before every possible
   * exit point. It behaves in almost all other respects like a standard
   * pointer. For example:
   * \code
   * void my_function () {
   *   Ptr<Object> object (new Object);
   *
   *   object->member = something;       // member-by-pointer operator
   *   call_by_reference (*object);      // dereference operator
   *   call_by_pointer (object);         // get actual address of object by implicit casting
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
   * If the objects to be managed are actually arrays, then the \a is_array
   * template parameter should be set to \c true. For example:
   * \code
   * Ptr<Object,true> array (new Object[10]);
   *
   * array[2] = something;
   * call_by_reference (array[2]);
   * call_by_pointer (array + 2);
   * array = new Object [25];    // delete previous array and point to new one
   * \endcode
   *
   * \note Both the copy constructor and the assignment operator will create
   * deep copies of the object as referenced by the original Ptr (assuming
   * non-NULL). One exception to this is when handling arrays: in this case,
   * the pointer stored in the assigned Ptr will be NULL, and no deep copy will
   * occur. This is due to the fact that the size of the array is unknown, and
   * a deep copy of the full array is therefore impossible.
   *
   * \sa RefPtr A reference-counting shared pointer
   * \sa VecPtr A %vector of pointers %to objects
   */
  template <class T, bool is_array = false> class Ptr
  {
    public:
      explicit Ptr (T* p = NULL) throw () : ptr (p) { }
      //! destructor
      /*! if non-NULL, the managed object will be destroyed using the \c delete
       * operator. */
      ~Ptr () {
        dealloc();
      }


      //! copy constructor
      /*! This will create a deep copy of the object pointed to by \a R, and
       * point to this copy.
       * \note the new object is created using the new operator in conjunction
       * with the object's copy constructor. In other words, the operation is
       * equivalent to:
       * \code
       * Ptr<T> p (new T);
       * ...
       * Ptr<T> p2 (new T (*p)); // copy constructor
       * \endcode
       * \warning if operating on arrays (i.e. the \a is_array template
       * parameter is \c true), this operation will simply delete the object
       * currently pointed to by the Ptr<T>, and leave a NULL pointer; no deep copy
       * will take place. This is due to the fact that the size of the array is
       * unknown, making it impossible to create a copy of the full array. */
      Ptr (const Ptr& R) : ptr (!is_array && R ? new T (*R) : NULL) { };

      //! Assignment (copy) operator
      /*! This will free the object currently managed (if any), and manage a
       * deep copy of the object pointer to by \a R instead.
       * \note the same limitations apply as for the copy constructor. */
      Ptr& operator= (const Ptr& R) {
        dealloc();
        ptr = (!is_array && R ? new T (*R) : NULL);
        return *this;
      }

      //! Assignment operator
      /*! This will free the object currently managed (if any), and manage the
       * new object instead. */
      Ptr& operator= (T* p)   {
        dealloc();
        ptr = p;
        return *this;
      }

      T& operator*() const throw ()   {
        return *ptr;
      }

      T* operator->() const throw ()  {
        return ptr;
      }
      //! Return address of actual object
      operator T* () const throw () {
        return ptr;
      }
      //! Return address of actual object, and stop managing the object.
      /*! Use this function to stop the Ptr<T> class from managing the object.
       * The destructor will NOT call delete on the object when it goes out of
       * scope: the user is responsible for ensuring that the object is freed
       * via other means. */
      T* release () throw ()    {
        T* p (ptr);
        ptr = NULL;
        return (p);
      }

    private:
      T* ptr;

      void dealloc () {
        if (is_array)
          delete [] ptr;
        else
          delete ptr;
      }
  };




  //! A shared reference-counting smart pointer implementation
  /*! A pointer class that will delete the object it points to in its
   * destructor if it is the last to point to the object.  This is useful to pass
   * pointers to different parts of the code whilst still ensuring
   * that allocated objects are eventually destroyed when all pointers that
   * reference it have been destroyed.  It behaves in almost all other
   * respects like a standard pointer. For example:
   * \code
   * void my_function () {
   *   RefPtr<Object> object (new Object);
   *
   *   object->member = something;       // member-by-pointer operator
   *   call_by_reference (*object);      // dereference operator
   *   call_by_pointer (object);         // get actual address of object by implicit casting
   *   object = new Object (parameters); // delete previous object and point to new one
   *
   *   if (something) return; // object goes out of scope: pointer is freed in destructor
   *
   *   RefPtr<Object> object2 (object);  // new reference to the same pointer
   *
   *   // deleting or reassigning either RefPtr at this point does not free the
   *   // object, since a reference to it still exists:
   *   object = NULL;                    // original pointer is not freed
   *
   *   if (error) throw Exception ("oops"); // pointer is freed in destructor
   *   ...
   * } // both RefPtr go out of scope: pointer is freed in destructor
   * \endcode
   *
   * \note As for the Ptr<T> class, use the \a is_array template parameter when
   * handling arrays of objects. This ensures the array version of the \c
   * delete operator is appropriately called when the array is freed.
   *
   * \sa Ptr A simple smart pointer
   * \sa VecPtr A %vector of pointers %to objects
   */
  template <class T, bool is_array = false> class RefPtr
  {
    public:
      explicit RefPtr (T* p = NULL) throw () : ptr (p), count (new size_t) {
        *count = 1;
      }
      //! copy constructor
      /*! The newly-created instance will point to the same location as the
       * original. */
      RefPtr (const RefPtr& R) throw () : ptr (R.ptr), count (R.count) {
        ++*count;
      }
      //! destructor
      /*! if non-NULL and no other RefPtr references this location, the managed
       * object will be destroyed using the \c delete operator. */
      ~RefPtr () {
        if (*count == 1) {
          dealloc();
          delete count;
        }
        else --*count;
      }

      //! assignment operator
      /*! Drops any reference to an existing object (and deletes it if it is no
       * longer referenced by any other RefPtr), and point to the same location
       * as \a R. */
      RefPtr& operator= (const RefPtr& R) {
        if (this == &R)
          return *this;
        if (*count == 1) {
          dealloc();
          delete count;
        }
        else --*count;
        ptr = R.ptr;
        count = R.count;
        ++*count;
        return *this;
      }

      //! assignment operator
      /*! Drops any reference to an existing object (and deletes it if it is no
       * longer referenced by any other RefPtr), and point to \a p. */
      RefPtr& operator= (T* p) {
        if (ptr == p)
          return *this;
        if (*count == 1) dealloc();
        else {
          --*count;
          count = new size_t;
          *count = 1;
        }
        ptr = p;
        return *this;
      }

      T& operator*() const throw ()   {
        return *ptr;
      }
      T* operator->() const throw ()  {
        return ptr;
      }
      //! Return address of actual object
      operator T* () const throw () {
        return ptr;
      }

      //! required to handle pointers to derived classes
      template <class U, bool> friend class RefPtr;
      //! required to handle pointers to derived classes
      template <class U> RefPtr (const RefPtr<U,is_array>& R) throw () : ptr (R.ptr), count (R.count) {
        ++*count;
      }
      //! required to handle pointers to derived classes
      template <class U> RefPtr<U,is_array>& operator= (const RefPtr<U,is_array>& R) {
        if (this == &R)
          return *this;
        if (*count == 1) {
          dealloc();
          delete count;
        }
        else --*count;
        ptr = R.ptr;
        count = R.count;
        ++*count;
        return *this ;
      }

      friend std::ostream& operator<< (std::ostream& stream, const RefPtr<T,is_array>& R) {
        stream << "(" << R.ptr << "): ";
        if (R)
          stream << *R.ptr;
        else
          stream << "null";
        stream << " (" << *R.count << " refs, counter at " << R.count << ")";
        return stream;
      }

    private:
      T*      ptr;
      size_t* count;

      void dealloc () {
        if (is_array)
          delete [] ptr;
        else
          delete ptr;
      }
  };






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
   * \sa RefPtr A reference-counting shared pointer
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
        V.insert (position, NULL);
        *position = item;
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


