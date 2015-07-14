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

#ifndef __algo_threaded_loop_h__
#define __algo_threaded_loop_h__

#include "debug.h"
#include "algo/loop.h"
#include "algo/iterator.h"
#include "thread.h"

namespace MR
{

  /** \addtogroup Thread
   * @{
   *
   * \defgroup image_thread_looping Thread-safe image looping 
   * \brief A class to perform multi-threaded operations on images
   *
   * The ThreadedLoop class provides a simple and powerful interface
   * for rapid development of multi-threaded image processing applications.
   * For full details, please refer the ThreadedLoop class
   * documentation.
   *
   * @} */

  /*! \addtogroup loop 
   * \{ */

  //! a class to loop over images in a multi-threaded fashion
  /*! \ingroup image_thread_looping 
   * This class allows arbitrary looping operations to be performed in
   * parallel, using a versatile multi-threading framework. It builds on the
   * single-threaded imaging looping classes, Loop and
   * LoopInOrder, and can be used to code up complex operations with
   * relatively little effort.
   *
   * The ThreadedLoop class is generally used by first initialising
   * the class to determine the order of traversal, which axes will be looped
   * over between synchronisation calls, and what message to display in the
   * progress bar if one is needed. This is all performed in the various
   * constructors, which are template functions allowing the class to be
   * initialised from any InfoType class. These are described in more detail
   * below.
   *
   * A number of methods are then available to loop over ImageType classes.
   * To do this, the ThreadedLoop class defines a set of axes that
   * each thread independently manages - each thread is responsible for 
   * looping over these axes. These axes can be queried after initialisation
   * of the ThreadedLoop using the inner_axes() method. By default,
   * this consists of a single axis, and is chosen to be the axis of smallest
   * stride in the \a source InfoType class provided at initialisation. The
   * remaining axes are managed by the ThreadedLoop class: each
   * invocation of the thread's functor is given a fresh position to operate
   * from, in the form of an Iterator class.
   *
   * Conceptually, the ThreadedLoop performs something akin to the
   * following:
   *
   * \code
   * // a generic ImageType object to be looped over:
   * ImageType vox;
   *
   * // the function or functor to be invoked:
   * void my_func (ImageType& vox) {
   *   // do something
   * }
   *
   * // outer loop includes axes 1 and above:
   * Loop outer_loop (vox, 1); 
   *
   * // inner loop includes axis 0:
   * Loop inner_loop (vox, 0, 1);
   *
   * // the outer loop is processed sequentially in a thread-safe manner,
   * // so that each thread receives the next position in the loop:
   * for (auto i = outer_loop.run (vox); i; ++i) {
   *   // the inner loop is processed within each thread, with multiple threads 
   *   // running concurrently, each processing a different row of voxels:
   *   for (auto j = inner_loop.run (vox); j; ++j)
   *     my_func (vox);
   * }
   * \endcode
   */
  /* \section threaded_loop_constructor Constructors
   *
   * The ThreadedLoop constructors can be used to set up any
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
   * recommended to provide the (or one of the) \e input ImageType class that is to be
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
   * The run() methods will run the ThreadedLoop, invoking the
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
   * ThreadedLoop ("computing exponential in-place...", vox)
   *   .run (my_function, vox);
   * \endcode
   */
  /* To make this operation more easily generalisable to any ImageType, use a
   * functor with a template operator() method:
   *
   * \code
   * class MyFunction {
   *   public: 
   *     template <class ImageType> 
   *       void operator() (ImageType& vox) {
   *         vox.value() = std::exp (vox.value());
   *       }
   * };
   *
   * ...
   *
   * AnyVoxelType vox;
   * ThreadedLoop ("computing exponential in-place...", vox)
   *   .run (MyFunction(), vox);
   * \endcode
   *
   * Note that a simple operation such as the previous example can be written
   * more compactly using C++11 lambda expressions:
   * \code
   * MyVoxelType vox;
   * ThreadedLoop ("computing exponential in-place...", vox)
   *   .run ([](decltype(vox)& v) { v.value() = std::exp(v.value()); }, vox);
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
   * ThreadedLoop (vox1).run (MyAdd(), vox_out, vox1, vox2); 
   * \endcode 
   *
   * Again, such a simple operation can be written more compactly using C++11 lambda expressions:
   * \code
   * auto f = [](decltype(vox_out)& out, decltype(vox1)& in1, decltype(vox2)& in2) {
   *   out.value() = in1.value() + in2.value();
   * }
   * ThreadedLoop (vox1).run (f, vox_out, vox1, vox2);
   * \endcode
   * 
   */ 
  /* This example uses a functor to computes the root-mean-square of \a vox:
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
   *     template <class ImageType>
   *       void operator() (const ImageType& in) { 
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
   * ThreadedLoop ("computing RMS of \"" + vox.name() + "\"...", vox)
   *     .run (RMS(SoS), vox);
   *
   * double rms = std::sqrt (SoS / voxel_count (vox));
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
   * \sa Loop
   * \sa ThreadedLoop
   */



  namespace {

    inline std::vector<size_t> get_inner_axes (const std::vector<size_t>& axes, size_t num_inner_axes) {
      return { axes.begin(), axes.begin()+num_inner_axes };
    }

    inline std::vector<size_t> get_outer_axes (const std::vector<size_t>& axes, size_t num_inner_axes) {
      return { axes.begin()+num_inner_axes, axes.end() };
    }

    template <class HeaderType>
      inline std::vector<size_t> get_inner_axes (const HeaderType& source, size_t num_inner_axes, size_t from_axis, size_t to_axis) {
        return get_inner_axes (Stride::order (source, from_axis, to_axis), num_inner_axes);
      }

    template <class HeaderType>
      inline std::vector<size_t> get_outer_axes (const HeaderType& source, size_t num_inner_axes, size_t from_axis, size_t to_axis) {
        return get_outer_axes (Stride::order (source, from_axis, to_axis), num_inner_axes);
      }


    template <int N, class Functor, class... ImageType>
      struct ThreadedLoopRunInner
      {
        const std::vector<size_t>& outer_axes;
        decltype (Loop (outer_axes)) loop;
        typename std::remove_reference<Functor>::type func;
        std::tuple<ImageType...> vox;

        ThreadedLoopRunInner (const std::vector<size_t>& outer_axes, const std::vector<size_t>& inner_axes,
            const Functor& functor, ImageType&... voxels) :
          outer_axes (outer_axes),
          loop (Loop (inner_axes)),
          func (functor), 
          vox (voxels...) { }

        void operator() (const Iterator& pos) {
          assign_pos_of (pos, outer_axes).to (vox);
          for (auto i = unpack (loop, vox); i; ++i) 
            unpack (func, vox);
        }
      };


    template <class Functor, class... ImageType>
      struct ThreadedLoopRunInner<0,Functor,ImageType...>
      {
        const std::vector<size_t>& outer_axes;
        decltype (Loop (outer_axes)) loop;
        typename std::remove_reference<Functor>::type func;

        ThreadedLoopRunInner (const std::vector<size_t>& outer_axes, const std::vector<size_t>& inner_axes,
            const Functor& functor, ImageType&... voxels) :
          outer_axes (outer_axes),
          loop (Loop (inner_axes)),
          func (functor) { }

        void operator() (Iterator& pos) {
          for (auto i = loop (pos); i; ++i)
            func (pos);
        }
      };


    template <class OuterLoopType>
      struct ThreadedLoopRunOuter {
        Iterator iterator;
        OuterLoopType outer_loop;
        std::vector<size_t> inner_axes;

        //! invoke \a functor (const Iterator& pos) per voxel <em> in the outer axes only</em>
        template <class Functor> 
          void run_outer (Functor&& functor)
          {
            if (Thread::number_of_threads() == 0) {
              for (auto i = outer_loop (iterator); i; ++i)
                functor (iterator);
              return;
            }

            struct Shared {
              Iterator& iterator;
              decltype (outer_loop (iterator)) loop;
              std::mutex mutex;
              FORCE_INLINE bool next (Iterator& pos) {
                std::lock_guard<std::mutex> lock (mutex);
                if (loop) {
                  assign_pos_of (iterator, loop.axes).to (pos);
                  ++loop;
                  return true;
                }
                else return false;
              }
            } shared = { iterator, outer_loop (iterator) };

            struct {
              Shared& shared;
              typename std::remove_reference<Functor>::type func;
              void execute () {
                Iterator pos = shared.iterator;
                while (shared.next (pos))
                  func (pos);
              }
            } loop_thread = { shared, functor };

            auto t = Thread::run (Thread::multi (loop_thread), "loop threads");
            t.wait();
          }



        //! invoke \a functor (const Iterator& pos) per voxel <em> in the outer axes only</em>
        template <class Functor, class... ImageType>
          void run (Functor&& functor, ImageType&&... vox)
          {
            ThreadedLoopRunInner< 
              sizeof...(ImageType),
              typename std::remove_reference<Functor>::type, 
              typename std::remove_reference<ImageType>::type...
                > loop_thread (outer_loop.axes, inner_axes, functor, vox...);
            run_outer (loop_thread);
          }

      };
  }







  template <class HeaderType>
    inline ThreadedLoopRunOuter<decltype(Loop(std::vector<size_t>()))> ThreadedLoop (
        const HeaderType& source,
        const std::vector<size_t>& outer_axes,
        const std::vector<size_t>& inner_axes) {
      return { source, Loop (outer_axes), inner_axes };
    }


  template <class HeaderType>
    inline ThreadedLoopRunOuter<decltype(Loop(std::vector<size_t>()))> ThreadedLoop (
        const HeaderType& source,
        const std::vector<size_t>& axes,
        size_t num_inner_axes = 1) {
      return { source, Loop (get_outer_axes (axes, num_inner_axes)), get_inner_axes (axes, num_inner_axes) }; 
    }

  template <class HeaderType>
    inline ThreadedLoopRunOuter<decltype(Loop(std::vector<size_t>()))> ThreadedLoop (
        const HeaderType& source,
        size_t from_axis = 0,
        size_t to_axis = std::numeric_limits<size_t>::max(),
        size_t num_inner_axes = 1) {
      return { source, 
        Loop (get_outer_axes (source, num_inner_axes, from_axis, to_axis)), 
        get_inner_axes (source, num_inner_axes, from_axis, to_axis) };
      }

  template <class HeaderType>
    inline ThreadedLoopRunOuter<decltype(Loop("", std::vector<size_t>()))> ThreadedLoop (
        const std::string& progress_message,
        const HeaderType& source,
        const std::vector<size_t>& outer_axes,
        const std::vector<size_t>& inner_axes) {
      return { source, Loop (progress_message, outer_axes), inner_axes };
    }

  template <class HeaderType>
    inline ThreadedLoopRunOuter<decltype(Loop("", std::vector<size_t>()))> ThreadedLoop (
        const std::string& progress_message,
        const HeaderType& source,
        const std::vector<size_t>& axes,
        size_t num_inner_axes = 1) {
      return { source, 
        Loop (progress_message, get_outer_axes (axes, num_inner_axes)),
        get_inner_axes (axes, num_inner_axes) };
      }

  template <class HeaderType>
    inline ThreadedLoopRunOuter<decltype(Loop("", std::vector<size_t>()))> ThreadedLoop (
        const std::string& progress_message,
        const HeaderType& source,
        size_t from_axis = 0,
        size_t to_axis = std::numeric_limits<size_t>::max(), 
        size_t num_inner_axes = 1) {
      return { source,
        Loop (progress_message, get_outer_axes (source, num_inner_axes, from_axis, to_axis)),
        get_inner_axes (source, num_inner_axes, from_axis, to_axis) };
      }


  /*! \} */

}

#endif


