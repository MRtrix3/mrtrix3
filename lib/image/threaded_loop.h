/*
    Copyright 2009 Brain Research Institute, Melbourne, Australia

    Written by J-Donald Tournier, 20/10/09.

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

#ifndef __image_threaded_loop_h__
#define __image_threaded_loop_h__

#include "debug.h"
#include "image/loop.h"
#include "image/iterator.h"
#include "thread/mutex.h"
#include "thread/exec.h"

namespace MR
{

  static const int Input = 0x01;
  static const int Output = 0x02;

  namespace Image
  {


    /*! \addtogroup loop 
     * \{ */

    //! a class to loop over images in a multi-threaded fashion
    /*! This class allows arbitrary looping operations to be performed in
     * parallel, using a versatile multi-threading framework. It builds on the
     * single-threaded imaging looping classes, Image::Loop and
     * Image::LoopInOrder, and can be used to code up complex operations with
     * relatively little effort.
     *
     * The Image::ThreadedLoop class is generally used by first initialising
     * the class to determine the order of traversal, which axes will be looped
     * over between synchronisation calls, and what message to display in the
     * progress bar if one is needed. This is all performed in the various
     * constructors, which are template functions allowing the class to be
     * initialised from any InfoType class. These are described in more detail
     * below.
     *
     * A number of methods are then available to loop over VoxelType classes.
     * To do this, the Image::ThreadedLoop class defines a set of axes that
     * each thread independently manages - each thread is responsible for 
     * looping over these axes. These axes can be queried after initialisation
     * of the Image::ThreadedLoop using the inner_axes() method. By default,
     * this consists of a single axis, and is chosen to be the axis of smallest
     * stride in the \a source InfoType class provided at initialisation. The
     * remaining axes are managed by the Image::ThreadedLoop class: each
     * invocation of the thread's functor is given a fresh position to operate
     * from, in the form of an Image::Iterator class.
     *
     *
     * \par The run_outer() method
     *
     * The most general of these methods is the run_outer() method, which launches a
     * set of threads that will each iteratively invokes the functor, providing
     * it with an Image::Iterator class containing a fresh voxel position to
     * process. This takes a single argument to a functor class that must
     * provide an operator() (const Image::Iterator& pos) member function. For
     * example, the following code can be used to compute the exponential of
     * the voxel values in-place. Note that this is for illustration purposes
     * only; this can be done in a much simpler way using the convenience
     * functions below.
     * \code
     * class MyFunctor {
     *   public:
     *     MyFunctor (ImageVoxelType& vox, const std::vector<size_t>& inner_axes) :
     *         vox (vox),
     *         loop (inner_axes) { }
     *     void operator() (const Image::Iterator& pos) {
     *       Image::voxel_assign (vox, pos);
     *       for (loop.start (vox); loop.ok(); loop.next (vox))
     *         vox.value() = Math::exp (vox.value());
     *     }
     *   protected:
     *     ImageVoxelType vox;
     *     Image::LoopInOrder loop;
     * };
     *
     * ...
     *
     * MyVoxelType vox;
     * Image::ThreadedLoop loop (vox);
     * class MyFunctor myfunc (vox, loop.inner_axes());
     * loop.run_outer (myfunc);
     * \endcode
     * Obviously, any number of VoxelType objects can be involved in the
     * computation, as long as they are members of the functor. For example, to
     * restrict the computation above to a binary mask:
     * \code
     * class MyFunctor {
     *   public:
     *     MyFunctor (ImageVoxelType& vox, MaskVoxelType& mask, const std::vector<size_t>& inner_axes) :
     *         vox (vox),
     *         mask (mask),
     *         loop (inner_axes) { }
     *     void operator() (const Image::Iterator& pos) {
     *       Image::voxel_assign (mask, pos);
     *       Image::voxel_assign (vox, pos);
     *       for (loop.start (mask, vox); loop.ok(); loop.next (mask, vox))
     *         vox.value() = mask.value() ? Math::exp (vox.value()) : NAN;
     *     }
     *   protected:
     *     ImageVoxelType vox;
     *     MaskVoxelType mask;
     *     Image::LoopInOrder loop;
     * };
     *
     * ...
     *
     * MyVoxelType vox;
     * MyMaskVoxelType mask;
     * Image::ThreadedLoop loop (vox);
     * MyFunctor myfunc (vox, mask, loop.inner_axes());
     * loop.run_outer (myfunc);
     * \endcode
     *
     *
     * \par The run() method
     *
     * The run() method is a convenience function that reduces the amount of
     * code required for most simple operations. Essentially, it takes a
     * simpler functor and manages looping over the inner axes. In this case,
     * the functor is invoked once per voxel, and is provided with a fresh
     * Image::Iterator each time. Using this method, the operation above can be
     * coded as:
     * \code
     * class MyFunctor {
     *   public:
     *     MyFunctor (ImageVoxelType& vox) : vox (vox) { }
     *     void operator() (const Image::Iterator& pos) {
     *       Image::voxel_assign (vox, pos);
     *       vox.value() = Math::exp (vox.value());
     *     }
     *   protected:
     *     ImageVoxelType vox;
     * };
     *
     * ...
     *
     * MyVoxelType vox;
     * Image::ThreadedLoop loop (vox).run (MyFunctor (vox));
     * \endcode
     *
     *
     * \par The run_foreach() methods
     *
     * These convenience functions can be used for any per-voxel operation, and
     * simplify the code further by taking a simple function, or a functor if
     * required, and managing all looping aspects and VoxelType interactions. 
     * These functions require a function or class as an argument that defines
     * the operation to be performed. When passing a class, the operator()
     * method will be invoked. The number and order of arguments of this
     * function should match the number of VoxelType objects supplied to
     * run_foreach(). 
     *
     * The run_foreach() functions take the following arguments:
     * - \a functor: the function or functor defining the operation itself.
     * - \a voxN: the VoxelType objects whose voxel values are to be
     * used as inputs/outputs; their number and order should match that of
     * the function or functor.
     * - \a flagsN: specifies whether the preceding VoxelType object is an
     * input and/or an output, in the form of an \a Input or \a Output flags
     * (or an OR'd combination of them). If \a Input is specified, the
     * corresponding value will be read from the VoxelType object before
     * invoking the function or functor. If \a Output is specified, the
     * corresponding value as modified by the function or functor will be
     * written to the VoxelType object for each voxel. In this case, you need
     * to ensure that the function or functor takes the corresponding argument
     * by reference.  Note that arguments can be specified to be both inputs
     * and outputs if required.
     *
     * For example, the example above, computing the exponential of \a vox
     * in-place, can be coded up very simply:
     * \code
     * void myfunc (float& val) { val = Math::exp (val); }
     *
     * ...
     *
     * Image::ThreadedLoop ("computing exponential in-place...", vox)
     *    .run_foreach (myfunc, vox, Input | output);
     * \endcode
     * 
     * As a further example, the following snippet performs the addition of \a
     * vox1 and \a vox2, this time storing the results in \a vox_out, with no
     * progress display, and looping according to the strides in \a vox1: 
     * \code
     * void myadd (float& out, float in1, float in2) { out = in1 + in2; }
     *
     * ...
     *
     * Image::ThreadedLoop (vox1)
     *     .run_foreach (myadd, 
     *               vox_out, Output, 
     *               vox1, Input, 
     *               vox2, Input);
     * \endcode 
     * 
     * This example uses a functor to computes the root-mean-square of \a vox,
     * with no per-voxel output:
     * \code 
     * class RMS {
     *   public:
     *     // We pass a reference to the same double to all threads. 
     *     // Each thread accumulates its own sum-of-squares, and 
     *     // updates the overal sum-of-squares in the destructor, which is
     *     // guaranteed to be invoked after all threads have re-joined,
     *     // thereby avoiding race conditions.
     *     RMS (double& grand_SoS) : SoS (0.0), grand_SoS (grand_SoS) { }
     *     ~RMS () { grand_SoS += SoS; }
     *
     *     // accumulate the thread-local sum-of-squares:
     *     void operator() (float in) { SoS += Math::pow2 (in); } 
     *
     *   protected:
     *     double SoS;
     *     double& grand_SoS;
     * };
     * 
     * ...
     *
     * double SoS = 0.0;
     * Image::ThreadedLoop ("computing RMS of \"" + vox.name() + "\"...", vox)
     *     .run_foreach (RMS(SoS), vox, Input);
     *
     * double rms = Math::sqrt (SoS / Image::voxel_count (vox));
     * \endcode
     *
     *
     * \par Constructors
     *
     * The Image::ThreadedLoop constructors can be used to set up any
     * reasonable multi-threaded looping structure. The various relevant
     * aspects that can be influenced include:
     * - which axes will be iterated over; by default, all axes of the \a
     * source InfoType class will be included.
     * - the order of traversal, in the form of a list of axes that will be
     * iterated over in order; the first entry in the list will be iterated
     * over in the inner-most loop, etc. By default, the axes are traversed in
     * the order of increasing stride of the \a source InfoType class.
     * - the number of axes that will be managed independently by each thread.
     * By default, this number is one.
     * - a string that will be displayed in the progress bar if one is desired.
     *
     * The various constructors allow these settings to be specified in
     * different ways. An InfoType class is always required; it is generally
     * recommended to provide the (or one of the) \e input VoxelType class that is to be
     * processed, since its order of traversal will have the most influence on
     * performance (by making the most efficient use of the hardware's RAM
     * access and CPU cache). It is also possible to supply a
     * std::vector<size_t> directly if required. 
     *
     * These axes can be restricted to a specific range to allow volume-wise
     * processing, etc.  The inner axes can be specified by supplying how many
     * are needed; they will then be taken from the list of axes to be looped
     * over. It is also possible to provide the list of inners axes and outer
     * axes as separate std::vector<size_t> arguments.
     *
     * \sa Image::Loop
     * \sa Image::ThreadedLoop
     */
    class ThreadedLoop
    {
      public:

        template <class InfoType>
          ThreadedLoop (
              const InfoType& source,
              const std::vector<size_t>& axes_out_of_thread,
              const std::vector<size_t>& axes_in_thread) :
            loop (axes_out_of_thread),
            dummy (source),
            axes (axes_in_thread) {
              loop.start (dummy);
            }

        template <class InfoType>
          ThreadedLoop (
              const InfoType& source,
              const std::vector<size_t>& axes_in_loop,
              size_t num_inner_axes = 1) :
            loop (__get_axes_out_of_thread (axes_in_loop, num_inner_axes)),
            dummy (source),
            axes (__get_axes_in_thread (axes_in_loop, num_inner_axes)) {
              loop.start (dummy);
            }

        template <class InfoType>
          ThreadedLoop (
              const InfoType& source,
              size_t num_inner_axes = 1,
              size_t from_axis = 0,
              size_t to_axis = std::numeric_limits<size_t>::max()) :
            loop (__get_axes_out_of_thread (source, num_inner_axes, from_axis, to_axis)),
            dummy (source),
            axes (__get_axes_in_thread (source, num_inner_axes, from_axis, to_axis)) {
              loop.start (dummy);
            }

        template <class InfoType>
          ThreadedLoop (
              const std::string& progress_message,
              const InfoType& source,
              const std::vector<size_t>& axes_out_of_thread,
              const std::vector<size_t>& axes_in_thread) :
            loop (axes_out_of_thread, progress_message),
            dummy (source),
            axes (axes_in_thread) {
              loop.start (dummy);
            }

        template <class InfoType>
          ThreadedLoop (
              const std::string& progress_message,
              const InfoType& source,
              const std::vector<size_t>& axes_in_loop,
              size_t num_inner_axes = 1) :
            loop (__get_axes_out_of_thread (axes_in_loop, num_inner_axes), progress_message),
            dummy (source),
            axes (__get_axes_in_thread (axes_in_loop, num_inner_axes)) {
              loop.start (dummy);
            }

        template <class InfoType>
          ThreadedLoop (
              const std::string& progress_message,
              const InfoType& source,
              size_t num_inner_axes = 1,
              size_t from_axis = 0,
              size_t to_axis = std::numeric_limits<size_t>::max()) :
            loop (__get_axes_out_of_thread (source, num_inner_axes, from_axis, to_axis), progress_message),
            dummy (source),
            axes (__get_axes_in_thread (source, num_inner_axes, from_axis, to_axis)) {
              loop.start (dummy);
            }

        const std::vector<size_t>& outer_axes () const { return loop.axes(); }
        const std::vector<size_t>& inner_axes () const { return axes; }
        const Iterator& iterator () const { return dummy; }

        bool next (Iterator& pos) {
          Thread::Mutex::Lock lock (mutex);
          if (loop.ok()) {
            loop.set_position (dummy, pos);
            loop.next (dummy);
            return true;
          }
          else return false;
        }

        template <class Functor> 
          void run_outer (Functor functor, const std::string& thread_label = "loop thread");

        template <class Functor> 
          void run (Functor functor, const std::string& thread_label = "loop thread");


        template <class Functor, class VoxelType1> 
          void run_foreach (Functor functor, VoxelType1& vox1, int flags1);

        template <class Functor, class VoxelType1, class VoxelType2> 
          void run_foreach (Functor functor, VoxelType1& vox1, int flags1, VoxelType2& vox2, int flags2);

        template <class Functor, class VoxelType1, class VoxelType2, class VoxelType3> 
          void run_foreach (Functor functor, VoxelType1& vox1, int flags1, VoxelType2& vox2, int flags2, VoxelType3& vox3, int flags3);

        template <class VoxelType>
          void set_outer_pos (VoxelType& vox, const Iterator& pos) const {
            for (size_t n = 0; n < loop.axes().size(); ++n)
              vox[loop.axes()[n]] = pos[loop.axes()[n]];
          }

      protected:
        LoopInOrder loop;
        Iterator dummy;
        const std::vector<size_t> axes;
        Thread::Mutex mutex;

        static std::vector<size_t> __get_axes_in_thread (
            const std::vector<size_t>& axes_in_loop,
            size_t num_inner_axes) {
          return std::vector<size_t> (axes_in_loop.begin(), axes_in_loop.begin()+num_inner_axes);
        }

        static std::vector<size_t> __get_axes_out_of_thread (
            const std::vector<size_t>& axes_in_loop,
            size_t num_inner_axes) {
          return std::vector<size_t> (axes_in_loop.begin()+num_inner_axes, axes_in_loop.end());
        }

        template <class InfoType>
          static std::vector<size_t> __get_axes_in_thread (
              const InfoType& source,
              size_t num_inner_axes,
              size_t from_axis,
              size_t to_axis) {
            return __get_axes_in_thread (Stride::order (source, from_axis, to_axis), num_inner_axes);
          }

        template <class InfoType>
          static std::vector<size_t> __get_axes_out_of_thread (
              const InfoType& source,
              size_t num_inner_axes,
              size_t from_axis,
              size_t to_axis) {
            return __get_axes_out_of_thread (Stride::order (source, from_axis, to_axis), num_inner_axes);
          }

    };


    class ThreadedLoopKernelBase {
      public:
        ThreadedLoopKernelBase (ThreadedLoop& shared_info) :
          shared (shared_info) { }

      protected:
        ThreadedLoop& shared;
    };


    template <class Functor>
      class ThreadedLoopKernelOuter : public ThreadedLoopKernelBase {
        public:
          ThreadedLoopKernelOuter (ThreadedLoop& shared_info, const Functor& functor) :
            ThreadedLoopKernelBase (shared_info),
            func (functor) { }

          void execute () {
            Iterator pos (shared.iterator());
            while (shared.next (pos))
              func (pos);
          }

        protected:
          Functor func;
      };


    template <class Functor>
      class ThreadedLoopKernelFull : public ThreadedLoopKernelBase {
        public:
          ThreadedLoopKernelFull (ThreadedLoop& shared_info, const Functor& functor) :
            ThreadedLoopKernelBase (shared_info),
            func (functor) { }

          void execute () {
            LoopInOrder loop (shared.inner_axes());
            Iterator pos (shared.iterator());
            while (shared.next (pos))
              for (loop.start (pos); loop.ok(); loop.next (pos))
                func (pos);
          }

        protected:
          Functor func;
      };


    template <class Functor, class VoxelType1>
      class ThreadedLoopKernelForEach1 : public ThreadedLoopKernelBase {
        public:
          ThreadedLoopKernelForEach1 (
              ThreadedLoop& shared_info, 
              const Functor& functor, 
              VoxelType1& vox1, int flags1) :
            ThreadedLoopKernelBase (shared_info), func (functor),
            vox1 (vox1),
            flags1 (flags1) { }

          void execute () {
            LoopInOrder loop (shared.inner_axes());
            Iterator pos (shared.iterator());
            typename VoxelType1::value_type val1 = typename VoxelType1::value_type();
            while (shared.next (pos)) {
              shared.set_outer_pos (vox1, pos);
              for (loop.start (vox1); loop.ok(); loop.next (vox1)) {
                if (flags1 & Input) val1 = vox1.value();
                func (val1);
                if (flags1 & Output) vox1.value() = val1;
              }
            }
          }

        protected:
          Functor func;
          VoxelType1 vox1;
          const int flags1;
      };



    template <class Functor, class VoxelType1, class VoxelType2>
      class ThreadedLoopKernelForEach2 : public ThreadedLoopKernelBase {
        public:
          ThreadedLoopKernelForEach2 (
              ThreadedLoop& shared_info, 
              const Functor& functor, 
              VoxelType1& vox1, int flags1, 
              VoxelType2& vox2, int flags2) :
            ThreadedLoopKernelBase (shared_info), func (functor),
            vox1 (vox1), vox2 (vox2),
            flags1 (flags1), flags2 (flags2) { }

          void execute () {
            LoopInOrder loop (shared.inner_axes());
            Iterator pos (shared.iterator());
            typename VoxelType1::value_type val1 = typename VoxelType1::value_type();
            typename VoxelType2::value_type val2 = typename VoxelType2::value_type();
            while (shared.next (pos)) {
              shared.set_outer_pos (vox1, pos);
              shared.set_outer_pos (vox2, pos);
              for (loop.start (vox1, vox2); loop.ok(); loop.next (vox1, vox2)) {
                if (flags1 & Input) val1 = vox1.value();
                if (flags2 & Input) val2 = vox2.value();
                func (val1, val2);
                if (flags1 & Output) vox1.value() = val1;
                if (flags2 & Output) vox2.value() = val2;
              }
            }
          }

        protected:
          Functor func;
          VoxelType1 vox1;
          VoxelType2 vox2;
          const int flags1, flags2;
      };


    template <class Functor, class VoxelType1, class VoxelType2, class VoxelType3>
      class ThreadedLoopKernelForEach3 : public ThreadedLoopKernelBase {
        public:
          ThreadedLoopKernelForEach3 (
              ThreadedLoop& shared_info, 
              const Functor& functor, 
              VoxelType1& vox1, int flags1, 
              VoxelType2& vox2, int flags2, 
              VoxelType3& vox3, int flags3)  :
            ThreadedLoopKernelBase (shared_info), func (functor),
            vox1 (vox1), vox2 (vox2), vox3 (vox3),
            flags1 (flags1), flags2 (flags2), flags3 (flags3) { }

          void execute () {
            LoopInOrder loop (shared.inner_axes());
            Iterator pos (shared.iterator());
            typename VoxelType1::value_type val1 = typename VoxelType1::value_type();
            typename VoxelType2::value_type val2 = typename VoxelType2::value_type();
            typename VoxelType3::value_type val3 = typename VoxelType3::value_type();
            while (shared.next (pos)) {
              shared.set_outer_pos (vox1, pos);
              shared.set_outer_pos (vox2, pos);
              shared.set_outer_pos (vox3, pos);
              for (loop.start (vox1, vox2, vox3); loop.ok(); loop.next (vox1, vox2, vox3)) {
                if (flags1 & Input) val1 = vox1.value();
                if (flags2 & Input) val2 = vox2.value();
                if (flags3 & Input) val3 = vox3.value();
                func (val1, val2, val3);
                if (flags1 & Output) vox1.value() = val1;
                if (flags2 & Output) vox2.value() = val2;
                if (flags3 & Output) vox3.value() = val3;
              }
            }
          }

        protected:
          Functor func;
          VoxelType1 vox1;
          VoxelType2 vox2;
          VoxelType3 vox3;
          const int flags1, flags2, flags3;
      };



    /*! \} */









    template <class Functor> 
      inline void ThreadedLoop::run_outer (Functor functor, const std::string& thread_label)
      {
        ThreadedLoopKernelOuter<Functor> loop_thread (*this, functor);
        Thread::Array<ThreadedLoopKernelOuter<Functor> > thread_list (loop_thread);
        Thread::Exec threads (thread_list, thread_label);
      }




    template <class Functor> 
      inline void ThreadedLoop::run (Functor functor, const std::string& thread_label)
      {
        ThreadedLoopKernelFull<Functor> loop_thread (*this, functor);
        Thread::Array<ThreadedLoopKernelFull<Functor> > thread_list (loop_thread);
        Thread::Exec threads (thread_list, thread_label);
      }



    template <class Functor, class VoxelType1> 
      inline void ThreadedLoop::run_foreach (Functor functor, VoxelType1& vox1, int flags1)
      {
        ThreadedLoopKernelForEach1<Functor, VoxelType1> 
          loop_thread (*this, functor, vox1, flags1);
        Thread::Array<ThreadedLoopKernelForEach1<Functor, VoxelType1> > 
          thread_list (loop_thread);
        Thread::Exec threads (thread_list, "run_foreach thread");
      }

    template <class Functor, class VoxelType1, class VoxelType2> 
      inline void ThreadedLoop::run_foreach (Functor functor, VoxelType1& vox1, int flags1, VoxelType2& vox2, int flags2)
      {
        ThreadedLoopKernelForEach2<Functor, VoxelType1, VoxelType2> 
          loop_thread (*this, functor, vox1, flags1, vox2, flags2);
        Thread::Array<ThreadedLoopKernelForEach2<Functor, VoxelType1, VoxelType2> > 
          thread_list (loop_thread);
        Thread::Exec threads (thread_list, "run_foreach thread");
      }

    template <class Functor, class VoxelType1, class VoxelType2, class VoxelType3> 
      inline void ThreadedLoop::run_foreach (Functor functor, VoxelType1& vox1, int flags1, VoxelType2& vox2, int flags2, VoxelType3& vox3, int flags3)
      {
        ThreadedLoopKernelForEach3<Functor, VoxelType1, VoxelType2, VoxelType3>
          loop_thread (*this, functor, vox1, flags1, vox2, flags2, vox3, flags3);
        Thread::Array<ThreadedLoopKernelForEach3<Functor, VoxelType1, VoxelType2, VoxelType3> >
          thread_list (loop_thread);
        Thread::Exec threads (thread_list, "run_foreach thread");
      }



  }
}

#endif


