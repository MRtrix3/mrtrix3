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

namespace MR {

  /** \addtogroup Memory 
   * @{ */

  //! The default de-allocator: it simply calls \c delete on \a p
  template <class T> class SingleDealloc {
    public:
      SingleDealloc () throw() { }
      SingleDealloc (const SingleDealloc& a) throw() { }
      template <class U> SingleDealloc (const SingleDealloc<U>& a) throw() { }
      ~SingleDealloc () throw() { }
      void operator() (T* p) { delete p; }
  };




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
  template <class T, class Dealloc = SingleDealloc<T> > class Ptr
  {
    public:
      explicit Ptr (T* p = NULL, const Dealloc& deallocator = Dealloc()) throw () : ptr (p), dealloc (deallocator) { }
      ~Ptr () { dealloc (ptr); }

      //! test whether the pointer is non-NULL
      operator bool() const throw ()       { return (ptr); }
      //! test whether the pointer is NULL
      bool     operator! () const throw () { return (!ptr); }

      //! Assignment operator
      /*! This will free the object currently pointed to by the Ptr<T> (if
       * any), and manage the new object instead. */
      Ptr&    operator= (T* p)   { dealloc (ptr); ptr = p; return (*this); }
      bool    operator== (const Ptr& R) const throw () { return (ptr == R.ptr); }
      bool    operator!= (const Ptr& R) const throw () { return (ptr != R.ptr); }
      T&      operator*() const throw ()   { return (*ptr); }
      T*      operator->() const throw ()  { return (ptr); }
      //! Return address of actual object
      T*      get () const throw ()        { return (ptr); }
      //! Return address of actual object, and stop managing the object.
      /*! Use this function to stop the Ptr<T> class from managing the object.
       * The destructor will NOT call delete on the object when it goes out of
       * scope: the user is responsible for ensuring that the object is freed
       * via other means. */
      T* release () const throw ()    { T* p (ptr); ptr = NULL; return (p); }

      //! comparison operator: calls the object's own comparison operator 
      bool    operator< (const Ptr& R) const { return (*ptr < *(R.ptr)); }

    private:
      T*   ptr;
      Dealloc dealloc;

      Ptr (const Ptr& R) : ptr (NULL) { assert (0); };
      Ptr& operator= (const Ptr& R) { assert (0); return (*this); }
  };





  template <class T, class Dealloc = SingleDealloc<T> > class RefPtr
  {
    public:
      explicit RefPtr (T* p = NULL, const Dealloc& deallocator = Dealloc()) throw () : ptr (p), count (new size_t), dealloc (deallocator) { *count = 1; }
      RefPtr (const RefPtr& R) throw () : ptr (R.ptr), count (R.count), dealloc (R.dealloc) { ++*count; }
      ~RefPtr () { if (*count == 1) { dealloc (ptr); delete count; } else --*count; }

      operator bool() const throw ()       { return (ptr); }
      bool     operator! () const throw () { return (!ptr); }

      RefPtr&    operator= (const RefPtr& R)
      { 
        if (this == &R) return (*this);
        if (*count == 1) { dealloc (ptr); delete count; } 
        else --*count; 
        ptr = R.ptr; count = R.count; ++*count;
        return (*this);
      }
      RefPtr&    operator= (T* p)
      {
        if (ptr == p) return (*this);
        if (*count == 1) dealloc (ptr); 
        else { --*count; count = new size_t; *count = 1; }
        ptr = p; 
        return (*this); 
      }

      bool    operator== (const RefPtr& R) const throw () { return (ptr == R.ptr); }
      bool    operator!= (const RefPtr& R) const throw () { return (ptr != R.ptr); }

      T&      operator*() const throw ()   { return (*ptr); }
      T*      operator->() const throw ()  { return (ptr); }
      T*      get() const throw ()         { return (ptr); }
      bool    unique() const throw ()      { return (*count == 1); }

      bool    operator< (const RefPtr& R) const { return (*ptr < *R.ptr); }

      template <class U, class V> friend class RefPtr;
      template <class U, class V> RefPtr (const RefPtr<U,V>& R) throw () : ptr (R.ptr), count (R.count), dealloc (R.dealloc) { ++*count; }
      template <class U, class V> RefPtr& operator= (const RefPtr<U,V>& R) 
      { 
        if (this == &R) return (*this);
        if (*count == 1) { dealloc (ptr); delete count; } else --*count; 
        ptr = R.ptr; count = R.count; ++*count; return (*this) ; 
      } 

      friend std::ostream& operator<< (std::ostream& stream, const RefPtr<T,Dealloc>& R)
      {
        stream << "(" << R.ptr << "): ";
        if (R) stream << *R.ptr;
        else stream << "null";
        stream << " (" << *R.count << " refs, counter at " << R.count << ")";
        return (stream);
      }

    private:
      T*      ptr;
      size_t* count;
      Dealloc dealloc;
  };


  template <class T, class Dealloc = SingleDealloc<T> > class VecPtr 
  {
    public:
      typedef typename std::vector<T*>::iterator iterator;
      typedef typename std::vector<T*>::const_iterator const_iterator;
      typedef typename std::vector<T*>::reference reference;
      typedef typename std::vector<T*>::const_reference const_reference;

      VecPtr (const Dealloc& deallocator = Dealloc()) : dealloc (deallocator) { }
      VecPtr (size_t num, const Dealloc deallocator = Dealloc()) : 
        V (num), dealloc (deallocator) { for (size_t i = 0; i < size(); ++i) V[i] = NULL; }
      ~VecPtr() { for (size_t i = 0; i < size(); ++i) dealloc (V[i]); }

      size_t size () const { return (V.size()); }
      bool empty () const { return (V.empty()); }
      reference operator[] (size_t i)       { return (V[i]); }
      const_reference operator[] (size_t i) const { return (V[i]); }

      void resize (size_t new_size) { 
        size_t old_size = size();
        for (size_t i = new_size; i < old_size; ++i) dealloc (V[i]); 
        V.resize (new_size);
        for (size_t i = old_size; i < new_size; ++i) V[i] = NULL;
      }

      iterator       begin()        { return (V.begin()); }
      const_iterator begin() const  { return (V.begin()); }
      iterator       end()          { return (V.end()); }
      const_iterator end() const    { return (V.end()); }

      reference       front ()       { return (V.front()); }
      const_reference front () const { return (V.front()); }
      reference       back ()        { return (V.back()); }
      const_reference back () const  { return (V.back()); }

      void push_back (T* item) { V.push_back (item); }
      void pop_back () { dealloc (V.back()); V.pop_back(); }

      iterator insert (iterator position, T* item = NULL) { V.insert (position, NULL); *position = item; }
      void insert (iterator position, size_t n) { V.insert (position, n, NULL); }


      iterator erase (iterator position) { dealloc (*position); return (V.erase (position)); }
      iterator erase (iterator first, iterator last) { for (iterator it = first; it != last; ++it) dealloc (*it); return (V.erase (first, last)); }

    private:
      std::vector<T*> V;
      Dealloc dealloc;
  };






  //! Equivalent classes for pointers to arrays
  /*! Use this construct when using the Ptr or RefPtr classes to refer to
   * array pointers. The difference between these and the regular Ptr anf
   * RefPtr classes is that the array versions use the delete [] operator.
   *
   * \code
   * Array<Item>::Ptr pointer (new Item [128]);
   * Array<Item>::RefPtr refpointer (new Item [12]);
   * \endcode
   * This is used instead of the equivalent non-array version:
   * \code
   * Ptr<Item> pointer (new Item);
   * RefPtr<Item> refpointer (new Item);
   * \endcode
   *
   * These Array versions ensure that the items are deallocated using the
   * array \c delete[] operator, rather than the single item \c delete
   * operator. Incorrect matching of \c new and \c delete can otherwise cause
   * programs to crash when \c delete is called.
   */
  template <class T> class Array {
    public:
      class Dealloc {
        public:
          void operator() (T* p) { delete [] p; }
      };
      typedef MR::Ptr<T, Dealloc>     Ptr;
      typedef MR::RefPtr<T, Dealloc>  RefPtr;
  };

  /** @} */
}

#endif


