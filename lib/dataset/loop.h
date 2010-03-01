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
#include "dataset/position.h"

namespace MR {
  namespace DataSet {

    //! \cond skip
    namespace {

      template <class Functor, class Set> class Kernel1 {
        public:
          Kernel1 (Functor& func, Set& set) : F (func), D (set) { assert (voxel_count (D)); }
          const std::string& name () { return (D.name()); }
          size_t ndim () const { return (D.ndim()); }
          ssize_t dim (size_t axis) const { return (D.dim (axis)); }
          Position<Kernel1<Functor,Set> > operator[] (size_t axis) {
            return (Position<Kernel1<Functor,Set> > (*this, axis)); }
          void check (size_t from_axis, size_t to_axis) { }
          void operator() () { F (D); }

          ssize_t get_pos (size_t axis) const { return (D[axis]); }
          void set_pos (size_t axis, ssize_t position) { D[axis] = position; }
          void move_pos (size_t axis, ssize_t inc) { D[axis] += inc; }
        private:
          Functor& F;
          Set& D;
      };

      template <class Functor, class Set, class Set2> class Kernel2 {
        public:
          Kernel2 (Functor& func, Set& set, Set2& set2) : F (func), D (set), D2 (set2) { assert (voxel_count (D)); }
          const std::string& name () { return (D.name()); }
          size_t ndim () const { return (D.ndim()); }
          ssize_t dim (size_t axis) const { return (D.dim (axis)); }
          Position<Kernel2<Functor,Set,Set2> > operator[] (size_t axis) {
            return (Position<Kernel2<Functor,Set,Set2> > (*this, axis)); }
          void check (size_t from_axis, size_t to_axis) const { 
            if (!dimensions_match (D, D2, from_axis, to_axis))
              throw Exception ("dimensions mismatch between \"" + D.name() + "\" and \"" + D2.name() + "\"");
          }
          void operator() () { F (D, D2); }

          ssize_t get_pos (size_t axis) const { return (D[axis]); }
          void set_pos (size_t axis, ssize_t position) { D[axis] = position; D2[axis] = position; }
          void move_pos (size_t axis, ssize_t inc) { D[axis] += inc; D2[axis] += inc; }
        private:
          Functor& F;
          Set& D;
          Set2& D2;
      };


      template <class FunctorKernel, class Mask> class KernelMask {
        public:
          KernelMask (FunctorKernel& func, Mask& mask) : F (func), M (mask) { assert (voxel_count (M)); }
          const std::string& name () { return (M.name()); }
          size_t ndim () const { return (M.ndim()); }
          ssize_t dim (size_t axis) const { return (M.dim (axis)); }
          Position<KernelMask<FunctorKernel,Mask> > operator[] (size_t axis) {
            return (Position<KernelMask<FunctorKernel,Mask> > (*this, axis)); }
          void check (size_t from_axis, size_t to_axis) const { 
            if (!dimensions_match (M, F, from_axis, to_axis))
              throw Exception ("dimensions mismatch between \"" + M.name() + "\" and \"" + F.name() + "\"");
          }
          void operator() () { 
            if (M.value()) { 
              for (size_t i = 0; i < M.ndim(); ++i) F[i] = M[i];
              F ();
            }
          }

          ssize_t get_pos (size_t axis) const { return (M[axis]); }
          void set_pos (size_t axis, ssize_t position) { M[axis] = position; }
          void move_pos (size_t axis, ssize_t inc) { M[axis] += inc; }
        private:
          FunctorKernel& F;
          Mask& M;
      };


      template <class Functor> class ProgressKernel {
        public:
          ProgressKernel (Functor& func, const std::string& message) : F (func), m (message) { }
          ~ProgressKernel () { ProgressBar::done(); }
          const std::string& name () { return (F.name()); }
          size_t ndim () const { return (F.ndim()); }
          ssize_t dim (size_t axis) const { return (F.dim (axis)); }
          Position<ProgressKernel<Functor> > operator[] (size_t axis) {
            return (Position<ProgressKernel<Functor> > (*this, axis)); }
          void check (size_t from_axis, size_t to_axis) const { 
            F.check (from_axis, to_axis);
            size_t count = 1;
            for (size_t i = from_axis; i < to_axis; ++i) count *= F.dim (i);
            ProgressBar::init (count, m);
          }
          void operator() () { F(); ProgressBar::inc(); }

          ssize_t get_pos (size_t axis) const { return (F[axis]); }
          void set_pos (size_t axis, ssize_t position) { F[axis] = position; }
          void move_pos (size_t axis, ssize_t inc) { F[axis] += inc; }
        private:
          Functor& F;
          const std::string& m;
      };


      template <class Kernel>
        inline void loop_exec (Kernel& K, size_t from_axis, size_t to_axis) {
          --to_axis;
          assert (K.dim(to_axis));
          K[to_axis] = 0;
loop_start:
          if (to_axis == from_axis) K();
          else loop_exec (K, from_axis, to_axis);
          if (K[to_axis] < K.dim(to_axis)-1) { ++K[to_axis]; goto loop_start; }
        }

    }
    //! \endcond 

    /** \addtogroup DataSet
     @{ */

    /** \defgroup loop Looping functions
     @{ */


    template <class Set> bool increment_position (Set& D, size_t from_axis, size_t to_axis) { 
      if (from_axis >= to_axis) return (false);
      if (D[from_axis]+1 < D.dim(from_axis)) { ++D[from_axis]; return (true); }
      D[from_axis] = 0; 
      return (increment_position (D, from_axis+1, to_axis));
    }

    template <class Set> inline bool increment_position (Set& D) { return (increment_position (D, 0, D.ndim())); }


    template <class Kernel> 
      void loop (Kernel& K, size_t from_axis = 0, size_t to_axis = SIZE_MAX) 
      {
        if (to_axis > K.ndim()) to_axis = K.ndim();
        assert (from_axis < to_axis);
        K.check (from_axis, to_axis);
        loop_exec (K, from_axis, to_axis);
      }


    template <class Functor, class Set> 
      void loop1 (Functor& func, Set& D, size_t from_axis = 0, size_t to_axis = SIZE_MAX) 
      {
        Kernel1<Functor,Set> kernel (func, D);
        loop (kernel, from_axis, to_axis);
      }

    template <class Functor, class Set> 
      void loop1 (const std::string& message, Functor& func, Set& D, size_t from_axis = 0, size_t to_axis = SIZE_MAX) 
      {
        typedef Kernel1<Functor,Set> MainKernel;
        MainKernel kernel (func, D);
        ProgressKernel<MainKernel> progress_kernel (kernel, message);
        loop (progress_kernel, from_axis, to_axis);
      }

    template <class Functor, class Set, class Set2> 
      void loop2 (Functor& func, Set& D, Set2& D2, size_t from_axis = 0, size_t to_axis = SIZE_MAX) 
      {
        Kernel2<Functor,Set,Set2> kernel (func, D, D2);
        loop (kernel, from_axis, to_axis);
      }

    template <class Functor, class Set, class Set2> 
      void loop2 (const std::string& message, Functor& func, Set& D, Set2& D2, size_t from_axis = 0, size_t to_axis = SIZE_MAX) 
      {
        typedef Kernel2<Functor,Set,Set2> MainKernel;
        MainKernel kernel (func, D, D2);
        ProgressKernel<MainKernel> progress_kernel (kernel, message);
        loop (progress_kernel, from_axis, to_axis);
      }


    template <class Functor, class Mask, class Set> 
      void loop1_mask (Functor& func, Mask& mask, Set& D, size_t from_axis = 0, size_t to_axis = 3) 
      {
        typedef Kernel1<Functor,Set> MainKernel;
        typedef KernelMask<MainKernel,Mask> MaskKernel;
        MainKernel main_kernel (func, D);
        MaskKernel mask_kernel (main_kernel, mask);
        loop (mask_kernel, from_axis, to_axis);
      }


    template <class Functor, class Mask, class Set> 
      void loop1_mask (const std::string& message, Functor& func, Mask& mask, Set& D, size_t from_axis = 0, size_t to_axis = 3) 
      {
        typedef Kernel1<Functor,Set> MainKernel;
        typedef KernelMask<MainKernel,Mask> MaskKernel;
        MainKernel main_kernel (func, D);
        MaskKernel mask_kernel (main_kernel, mask);
        ProgressKernel<MaskKernel> progress_kernel (mask_kernel, message);
        loop (progress_kernel, from_axis, to_axis);
      }

    //! @}
    //! @}
  }
}

#endif


