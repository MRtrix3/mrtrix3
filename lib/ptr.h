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

#include "mrtrix.h"

namespace MR {

  template <class T> class Ptr
  {
    public:
      explicit Ptr (T* p = NULL) throw () : ptr (p) { }
      ~Ptr () { if (ptr) delete ptr; }

      operator bool() const throw ()       { return (ptr); }
      bool     operator! () const throw () { return (!ptr); }

      Ptr&   operator= (T* p)   { if (ptr) delete ptr; ptr = p; return (*this); }
      bool    operator== (const Ptr& R) const throw () { return (ptr == R.ptr); }
      bool    operator!= (const Ptr& R) const throw () { return (ptr != R.ptr); }
      T&      operator*() const throw ()   { return (*ptr); }
      T*      operator->() const throw ()  { return (ptr); }
      T*      get () const throw ()        { return (ptr); }
      T*      release () const throw ()    { T* p (ptr); ptr = NULL; return (p); }

      bool    operator< (const Ptr& R) const { return (*ptr < *(R.ptr)); }

    private:
      T*   ptr;

      Ptr (const Ptr& R) : ptr (NULL) { assert (0); };
      Ptr& operator= (const Ptr& R) { assert (0); return (*this); }
  };






  template <class T> class RefPtr
  {
    public:
      explicit RefPtr (T* p = NULL) throw () : ptr (p), count (new size_t) { *count = 1; }
      RefPtr (const RefPtr& R) throw () : ptr (R.ptr), count (R.count) { ++*count; }
      ~RefPtr () { if (*count == 1) { delete ptr; delete count; } else --*count; }

      operator bool() const throw ()       { return (ptr); }
      bool     operator! () const throw () { return (!ptr); }

      RefPtr&    operator= (const RefPtr& R)
      { 
        if (this == &R) return (*this);
        if (*count == 1) { delete ptr; delete count; } 
        else --*count; 
        ptr = R.ptr; count = R.count; ++*count;
        return (*this);
      }
      RefPtr&    operator= (T* p)
      {
        if (ptr == p) return (*this);
        if (*count == 1) delete ptr; 
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

      template <class U> friend class RefPtr;
      template <class U> RefPtr (const RefPtr<U>& R) throw () : ptr (R.ptr), count (R.count) { ++*count; }
      template <class U> RefPtr& operator= (const RefPtr<U>& R) 
      { 
        if (this == &R) return (*this);
        if (*count == 1) { delete ptr; delete count; } else --*count; 
        ptr = R.ptr; count = R.count; ++*count; return (*this) ; 
      } 

      friend std::ostream& operator<< (std::ostream& stream, const RefPtr<T>& R)
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
  };

}

#endif


