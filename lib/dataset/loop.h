/*
    Copyright 2009 Brain Research Institute, Melbourne, Australia

    Written by J-Donald Tournier, 16/10/09.

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

#ifndef __dataset_loop_h__
#define __dataset_loop_h__

#include "progressbar.h"

namespace MR {
  namespace DataSet {

    //! \cond skip
    namespace {

      template <class Set> bool increment (Set& D, size_t axis, size_t last_axis) { 
        if (axis >= last_axis) return (false);
        if (D.pos(axis)+1 < ssize_t(D.dim(axis))) { D.move(axis,1); return (true); }
        D.pos (axis,0); 
        return (increment (D, axis+1, last_axis));
      }

      template <class Functor, class Set> class Kernel1 {
        public:
          Kernel1 (Functor& func, Set& set) : F (func), D (set) { assert (voxel_count (D)); }
          size_t ndim () const { return (D.ndim()); }
          ssize_t dim (size_t axis) const { return (D.dim (axis)); }
          ssize_t pos (size_t axis) const { return (D.pos (axis)); }
          void pos (size_t axis, ssize_t position) { D.pos (axis, position); }
          void move (size_t axis, ssize_t inc) { D.move (axis, inc); }
          void check (size_t from_axis, size_t to_axis) { }
          void operator() () { F (D); }
        private:
          Functor& F;
          Set& D;
      };

      template <class Functor, class Set, class Set2> class Kernel2 {
        public:
          Kernel2 (Functor& func, Set& set, Set2& set2) : F (func), D (set), D2 (set2) { assert (voxel_count (D)); }
          size_t ndim () const { return (D.ndim()); }
          ssize_t dim (size_t axis) const { return (D.dim (axis)); }
          ssize_t pos (size_t axis) const { return (D.pos (axis)); }
          void pos (size_t axis, ssize_t position) { D.pos (axis, position); D2.pos (axis, position); }
          void move (size_t axis, ssize_t inc) { D.move (axis, inc); D2.move (axis, inc); }
          void check (size_t from_axis, size_t to_axis) const { 
            for (size_t i = from_axis; i < to_axis; ++i) 
              if (D.dim (i) != D2.dim(i))
                throw Exception ("dimensions mismatch between \"" + D.name() + "\" and \"" + D2.name() + "\"");
          }
          void operator() () { F (D, D2); }
        private:
          Functor& F;
          Set& D;
          Set2& D2;
      };


      template <class Functor> class ProgressKernel {
        public:
          ProgressKernel (Functor& func, const std::string& message) : F (func), m (message) { }
          ~ProgressKernel () { ProgressBar::done(); }
          size_t ndim () const { return (F.ndim()); }
          ssize_t dim (size_t axis) const { return (F.dim (axis)); }
          ssize_t pos (size_t axis) const { return (F.pos (axis)); }
          void pos (size_t axis, ssize_t position) { F.pos (axis, position); }
          void move (size_t axis, ssize_t inc) { F.move (axis, inc); }
          void check (size_t from_axis, size_t to_axis) const { 
            F.check (from_axis, to_axis);
            size_t count = 1;
            for (size_t i = from_axis; i < to_axis; ++i) count *= F.dim (i);
            ProgressBar::init (count, m);
          }
          void operator() () { F(); ProgressBar::inc(); }
        private:
          Functor& F;
          const std::string& m;
      };
    }
    //! \endcond 

    //! \addtogroup DataSet
    // @{

    template <class Kernel> 
      void loop (Kernel& K, size_t from_axis = 0, size_t to_axis = SIZE_MAX) 
      {
        if (to_axis == SIZE_MAX) to_axis = K.ndim();
        assert (from_axis < to_axis);
        assert (to_axis <= K.ndim());
        K.check (from_axis, to_axis);
        for (size_t i = from_axis+1; i < to_axis; ++i) K.pos(i,0); 
        do {
          for (K.pos (from_axis,0); K.pos(from_axis) < K.dim(from_axis); K.move(from_axis,1)) K();
        } while (increment (K, from_axis+1, to_axis));
      }


    template <class Functor, class Set> 
      void loop1 (Functor& func, Set& D, size_t from_axis = 0, size_t to_axis = SIZE_MAX) 
      {
        Kernel1<Functor, Set> kernel (func, D);
        loop (kernel, from_axis, to_axis);
      }

    template <class Functor, class Set> 
      void loop1 (const std::string& message, Functor& func, Set& D, size_t from_axis = 0, size_t to_axis = SIZE_MAX) 
      {
        Kernel1<Functor, Set> kernel (func, D);
        ProgressKernel<Kernel1<Functor, Set> > progress (kernel, message);
        loop (progress, from_axis, to_axis);
      }

    template <class Functor, class Set, class Set2> 
      void loop2 (Functor& func, Set& D, Set2& D2, size_t from_axis = 0, size_t to_axis = SIZE_MAX) 
      {
        Kernel2<Functor, Set, Set2> kernel (func, D, D2);
        loop (kernel, from_axis, to_axis);
      }

    template <class Functor, class Set, class Set2> 
      void loop2 (const std::string& message, Functor& func, Set& D, Set2& D2, size_t from_axis = 0, size_t to_axis = SIZE_MAX) 
      {
        Kernel2<Functor, Set, Set2> kernel (func, D, D2);
        ProgressKernel<Kernel2<Functor, Set, Set2> > progress (kernel, message);
        loop (progress, from_axis, to_axis);
      }

    //! @}
  }
}

#endif


