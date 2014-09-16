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

  class Input {
    public:
      template <class VoxelType>
        void read (typename VoxelType::value_type& val, VoxelType& vox) const { val = vox.value(); }
      template <class VoxelType>
        void write (VoxelType& vox, const typename VoxelType::value_type& val) const { }
  };

  class InputOutput {
    public:
      template <class VoxelType>
        void read (typename VoxelType::value_type& val, VoxelType& vox) const { val = vox.value(); }
      template <class VoxelType>
        void write (VoxelType& vox, const typename VoxelType::value_type& val) const { vox.value() = val; }
  };

  class Output {
    public:
      template <class VoxelType>
        void read (typename VoxelType::value_type& val, VoxelType& vox) const { }
      template <class VoxelType>
        void write (VoxelType& vox, const typename VoxelType::value_type& val) const { vox.value() = val; }
  };




  namespace Image
  {

    /** \addtogroup Thread
     * @{
     *
     * \defgroup image_thread_looping Thread-safe image looping 
     * \brief A class to perform multi-threaded operations on images
     *
     * The Image::ThreadedLoop class provides a simple and powerful interface
     * for rapid development of multi-threaded image processing applications.
     * For full details, please refer the Image::ThreadedLoop class
     * documentation.
     *
     * @} */



    /*! \addtogroup loop 
     * \{ */

    //! a class to loop over images in a multi-threaded fashion
    /*! \ingroup image_thread_looping 
     * This class allows arbitrary looping operations to be performed in
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
     * Conceptually, the Image::ThreadedLoop performs something akin to the
     * following:
     *
     * \code
     * // a generic VoxelType object to be looped over:
     * VoxelType vox;
     *
     * // the function or functor to be invoked:
     * void my_func (VoxelType& vox) {
     *   // do something
     * }
     *
     * // outer loop includes axes 1 and above:
     * Image::Loop outer_loop (vox, 1); 
     *
     * // inner loop includes axis 0:
     * Image::Loop inner_loop (vox, 0, 1);
     *
     * // the outer loop is processed sequentially in a thread-safe manner,
     * // so that each thread receives the next position in the loop:
     * for (auto i = outer_loop (vox); i; ++i) {
     *   // the inner loop is processed within each thread, with multiple threads 
     *   // running concurrently, each processing a different row of voxels:
     *   for (auto j = inner_loop (vox); j; ++j)
     *     my_func (vox);
     * }
     * \endcode
     *
     * \section threaded_loop_constructor Constructors
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
     *
     *
     * \section threaded_loop_run The run() methods
     *
     * The run() methods will run the Image::ThreadedLoop, invoking the
     * specified function or functor once per voxel in the loop. The different
     * versions will expect different signatures for the function or functor,
     * and manage looping over different numbers of VoxelTypes. This should be
     * clarified by examining the examples below.
     *
     * This example can be used to compute the exponential of the voxel values
     * in-place, while displaying a progress message:
     *
     * \code
     * void my_function (MyVoxelType& vox) {
     *   vox.value() = std::exp (vox.value());
     * }
     *
     * ...
     *
     * MyVoxelType vox;
     * Image::ThreadedLoop ("computing exponential in-place...", vox)
     *   .run (my_function, vox);
     * \endcode
     *
     * To make this operation more easily generalisable to any VoxelType, use a
     * functor with a template operator() method:
     *
     * \code
     * class MyFunction {
     *   public: 
     *     template <class VoxelType> 
     *       void operator() (VoxelType& vox) {
     *         vox.value() = std::exp (vox.value());
     *       }
     * };
     *
     * ...
     *
     * AnyVoxelType vox;
     * Image::ThreadedLoop ("computing exponential in-place...", vox)
     *   .run (MyFunction(), vox);
     * \endcode
     *
     *
     * As a further example, the following snippet performs the addition of any
     * VoxelTypes \a vox1 and \a vox2, this time storing the results in \a
     * vox_out, with no progress display, and looping according to the strides
     * in \a vox1: 
     *
     * \code
     * struct MyAdd {
     *   template <class VoxelType1, class VoxelType2, class VoxelType3>
     *     void operator() (VoxelType1& out, const VoxelType2& in1, const VoxelType3& in2) {
     *       out.value() = in1.value() + in2.value(); 
     *     }
     * };
     *
     * ...
     *
     * Image::ThreadedLoop (vox1).run (MyAdd(), vox_out, vox1, vox2); 
     * \endcode 
     * 
     * 
     * This example uses a functor to computes the root-mean-square of \a vox:
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
     *     template <class VoxelType>
     *       void operator() (const VoxelType& in) { 
     *         SoS += Math::pow2 (in.value()); 
     *       } 
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
     *     .run (RMS(SoS), vox);
     *
     * double rms = std::sqrt (SoS / Image::voxel_count (vox));
     * \endcode
     *
     * \section threaded_loop_run_outer The run_outer() method
     *
     * The run_outer() method can be used if needed to loop over the indices in
     * the outer loop only. This method is used by the run() methods, and may
     * prove useful in selected cases if each thread needs to handle its own
     * looping over the inner axes. Usage is essentially identical to the run()
     * method, and the function or functor provided will need to have a void
     * operator() (const Iterator& pos) method defined.
     *
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
            }

        template <class InfoType>
          ThreadedLoop (
              const InfoType& source,
              const std::vector<size_t>& axes_in_loop,
              size_t num_inner_axes = 1) :
            loop (__get_axes_out_of_thread (axes_in_loop, num_inner_axes)),
            dummy (source),
            axes (__get_axes_in_thread (axes_in_loop, num_inner_axes)) {
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
            }

       
        //! all axes to be looped over
        std::vector<size_t> all_axes () const {
          std::vector<size_t> a (inner_axes());
          a.insert (a.end(), outer_axes().begin(), outer_axes().end());
          return a;
        }
        //! return an ordered vector of axes in the outer loop
        const std::vector<size_t>& outer_axes () const { return loop.axes(); }
        //! return an ordered vector of axes in the inner loop
        const std::vector<size_t>& inner_axes () const { return axes; }
        //! a dummy object that can be used to construct other Iterators
        const Iterator& iterator () const { return dummy; }

        void start (Iterator& pos) {
          loop.start (std::forward_as_tuple (pos));
        }

        //! get next position in the outer loop
        bool next (Iterator& pos) {
          Thread::Mutex::Lock lock (mutex);
          if (loop.ok()) {
            loop.set_position (dummy, pos);
            loop.next (std::forward_as_tuple (dummy));
            return true;
          }
          else return false;
        }

        //! invoke \a functor (const Iterator& pos) per voxel <em> in the outer axes only</em>
        template <class Functor> 
          void run_outer (Functor functor, const std::string& thread_label = "loop thread");

        //! invoke \a functor(const Iterator& pos) per voxel
        template <class Functor> 
          void run (Functor functor);

        //! invoke \a functor(VoxelType1& vox1) per voxel
        template <class Functor, class VoxelType1> 
          void run (Functor functor, VoxelType1& vox1);

        //! invoke \a functor(VoxelType1& vox1, VoxelType2& vox2) per voxel
        template <class Functor, class VoxelType1, class VoxelType2> 
          void run (Functor functor, VoxelType1& vox1, VoxelType2& vox2);

        //! invoke \a functor(VoxelType1& vox1, VoxelType2& vox2, VoxelType3& vox3) per voxel
        template <class Functor, class VoxelType1, class VoxelType2, class VoxelType3> 
          void run (Functor functor, VoxelType1& vox1, VoxelType2& vox2, VoxelType3& vox3);


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





     namespace {


       template <class Functor>
         class __Outer {
           public:
             __Outer (ThreadedLoop& shared_info, const Functor& functor) :
               shared (shared_info),
               func (functor) { }

             void execute () {
               Iterator pos (shared.iterator());
               shared.start (pos);
               while (shared.next (pos))
                 func (pos);
             }

           protected:
             ThreadedLoop& shared;
             Functor func;
         };





       template <class Functor>
         class __Run {
           public:
             __Run (ThreadedLoop& shared_info, const Functor& functor) :
               func (functor), 
               loop (shared_info.inner_axes()),
               outer_axes (shared_info.outer_axes()) { }

             void operator() (Iterator& pos) {
               for (auto i = loop (pos); i; ++i)
                 func (pos);
             }

           protected:
             Functor func;
             LoopInOrder loop;
             const std::vector<size_t>& outer_axes;
         };



       template <class Functor, class VoxelType1>
         class __Run1 : public __Run<Functor> {
           public:
             __Run1 (ThreadedLoop& shared_info, const Functor& functor, VoxelType1& vox1) :
               __Run<Functor> (shared_info, functor),
               vox1 (vox1) { }

             void operator() (const Iterator& pos) {
               voxel_assign (vox1, pos, this->outer_axes);
               for (auto i = this->loop (vox1); i; ++i)
                 this->func (vox1);
             }

           protected:
             VoxelType1 vox1;
         };



       template <class Functor, class VoxelType1, class VoxelType2>
         class __Run2 : public __Run<Functor> {
           public:
             __Run2 (ThreadedLoop& shared_info, const Functor& functor, VoxelType1& vox1, VoxelType2& vox2) :
               __Run<Functor> (shared_info, functor),
               vox1 (vox1), vox2 (vox2) { }

             void operator() (const Iterator& pos) {
               voxel_assign2 (vox1, vox2, pos, this->outer_axes);
               for (auto i = this->loop (vox1, vox2); i; ++i)
                 this->func (vox1, vox2);
             }

           protected:
             VoxelType1 vox1;
             VoxelType2 vox2;
         };




       template <class Functor, class VoxelType1, class VoxelType2, class VoxelType3>
         class __Run3 : public __Run<Functor> {
           public:
             __Run3 (ThreadedLoop& shared_info, const Functor& functor, VoxelType1& vox1, VoxelType2& vox2, VoxelType3& vox3) :
               __Run<Functor> (shared_info, functor),
               vox1 (vox1), vox2 (vox2), vox3 (vox3) { }

             void operator() (const Iterator& pos) {
               voxel_assign3 (vox1, vox2, vox3, pos, this->outer_axes);
               for (auto i = this->loop (vox1, vox2, vox3); i; ++i)
                 this->func (vox1, vox2, vox3);
             }

           protected:
             VoxelType1 vox1;
             VoxelType2 vox2;
             VoxelType3 vox3;
         };


     }



     /*! \} */









     template <class Functor> 
       inline void ThreadedLoop::run_outer (Functor functor, const std::string& thread_label)
       {
         if (Thread::number_of_threads() == 0) {
           for (auto i = loop (dummy); i; ++i)
             functor (dummy);
           return;
         }

         __Outer<Functor> loop_thread (*this, functor);
         Thread::Array<__Outer<Functor> > thread_list (loop_thread);
         Thread::Exec threads (thread_list, thread_label);
       }




     template <class Functor> 
       inline void ThreadedLoop::run (Functor functor)
       {
         if (Thread::number_of_threads() == 0) {
           LoopInOrder inner_loop (axes);
           for (auto i = loop (dummy); i; ++i) {
             for (auto j = inner_loop (dummy); j; ++j)
               functor (dummy);
           }
           return;
         }

         __Run<Functor> loop_thread (*this, functor);
         run_outer (loop_thread, "run thread");
       }


     template <class Functor, class VoxelType1> 
       void ThreadedLoop::run (Functor functor, VoxelType1& vox1)
       {
         if (Thread::number_of_threads() == 0) {
           LoopInOrder inner_loop (axes);
           for (auto i = loop (vox1); i; ++i) {
             for (auto j = inner_loop (vox1); j; ++j)
               functor (vox1);
           }
           return;
         }

         __Run1<Functor, VoxelType1> 
           loop_thread (*this, functor, vox1);
         run_outer (loop_thread, "run thread");
       }



     template <class Functor, class VoxelType1, class VoxelType2> 
       void ThreadedLoop::run (Functor functor, VoxelType1& vox1, VoxelType2& vox2)
       {
         if (Thread::number_of_threads() == 0) {
           LoopInOrder inner_loop (axes);
           for (auto i = loop (vox1, vox2); i; ++i) {
             for (auto j = inner_loop (vox1, vox2); j; ++j)
               functor (vox1, vox2);
           }
           return;
         }

         __Run2<Functor, VoxelType1, VoxelType2> 
           loop_thread (*this, functor, vox1, vox2);
         run_outer (loop_thread, "run thread");
       }



     template <class Functor, class VoxelType1, class VoxelType2, class VoxelType3> 
       void ThreadedLoop::run (Functor functor, VoxelType1& vox1, VoxelType2& vox2, VoxelType3& vox3)
       {
         if (Thread::number_of_threads() == 0) {
           LoopInOrder inner_loop (axes);
           for (auto i = loop (vox1, vox2, vox3); i; ++i) {
             for (auto j = inner_loop (vox1, vox2, vox3); j; ++j)
               functor (vox1, vox2, vox3);
           }
           return;
         }

         __Run3<Functor, VoxelType1, VoxelType2, VoxelType3> 
           loop_thread (*this, functor, vox1, vox2, vox3);
         run_outer (loop_thread, "run thread");
       }




  }
}

#endif


