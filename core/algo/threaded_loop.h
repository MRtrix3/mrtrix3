/* Copyright (c) 2008-2024 the MRtrix3 contributors.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Covered Software is provided under this License on an "as is"
 * basis, without warranty of any kind, either expressed, implied, or
 * statutory, including, without limitation, warranties that the
 * Covered Software is free of defects, merchantable, fit for a
 * particular purpose or non-infringing.
 * See the Mozilla Public License v. 2.0 for more details.
 *
 * For more details, see http://www.mrtrix.org/.
 */

#ifndef __algo_threaded_loop_h__
#define __algo_threaded_loop_h__

#include "algo/iterator.h"
#include "algo/loop.h"
#include "debug.h"
#include "mutexprotected.h"
#include "thread.h"
#include <tuple>

namespace MR {

/** \addtogroup thread_classes
 * @{
 *
 * \defgroup image_thread_looping Thread-safe image looping
 *
 * These functions allows arbitrary looping operations to be performed in
 * parallel, using a versatile multi-threading framework. It works
 * hand-in-hand with the [single-threaded looping functions](@ref MR::Loop()),
 * and can be used to code up complex operations with relatively little
 * effort.
 *
 * The ThreadedLoop() function is generally used by first initialising
 * an object that will determine the order of traversal, which axes will be
 * looped over between synchronisation calls, and what message to display in
 * the progress bar if one is needed. This is all performed in the various
 * overloaded versions of ThreadedLoop(), which are template functions
 * allowing the class to be initialised from any `HeaderType` class. These are
 * described in more detail below.
 *
 * The object returned by the ThreadedLoop() functions provide methods to
 * loop over ImageType classes.  To do this, the object returned by
 * ThreadedLoop() keeps track of which axes will be managed by each thread
 * independently (the _inner axes_), and which will be coordinated across
 * threads (the _outer axes_).  By default, the inner axes consist of a
 * single axis, chosen to be the axis of smallest stride in the \a source
 * `HeaderType` class provided at initialisation. The remaining axes are
 * coordinated across threads: each invocation of the thread's functor is
 * given a fresh position to operate from, in the form of an Iterator class.
 *
 * To help illustrate the concept, consider an example whereby the inner axes
 * have been set to the x & y axes (i.e. axes 0 & 1), and the outer axes have
 * been set to the z and volume axes (i.e. axes 2 & 3). Each thread will do
 * the following:
 *
 * 1. lock the loop mutex, obtain a new set of z & volume coordinates, then
 *    release the mutex so other threads can obtain their own unique set of
 *    coordinates to process;
 * 2. set the position of all `ImageType` classes to be processed according
 *    to these coordinates;
 * 3. iterate over the x & y axes, invoking the user-supplied functor each
 *    time;
 * 4. repeat from step 1 until all the data have been processed.
 *
 *
 * \section threaded_loop_constructor Instantiating a ThreadedLoop() object
 *
 * The ThreadedLoop() functions can be used to set up any reasonable
 * multi-threaded looping structure. The various relevant aspects that can be
 * influenced include:
 *
 * - which axes will be iterated over; by default, all axes of the \a source
 *   `HeaderType` class will be included.
 *
 * - the order of traversal, in the form of a list of axes that will be
 *   iterated over in order; the first entry in the list will be iterated
 *   over in the inner-most loop, etc. By default, the axes are traversed in
 *   the order of increasing stride of the \a source InfoType class.
 *
 * - the number of axes that will be managed independently by each thread.
 *   By default, this number is one.
 *
 * - a string that will be displayed in the progress bar if one is desired.
 *
 * The various overloaded functions allow these settings to be specified in
 * different ways. A `HeaderType` class is always required; it is generally
 * recommended to provide the (or one of the) \e input `ImageType` class that
 * is to be processed, since its order of traversal will have the most
 * influence on performance (by making the most efficient use of the
 * hardware's RAM access and CPU cache). It is also possible to supply a
 * `vector<size_t>` directly if required.
 *
 * These axes can be restricted to a specific range to allow volume-wise
 * processing, etc.  The inner axes can be specified by supplying how many
 * are needed; they will then be taken from the list of axes to be looped
 * over, in order of increasing stride. It is also possible to provide the
 * list of inners axes and outer axes as separate `vector<size_t>`
 * arguments.
 *
 *
 *
 * \section threaded_loop_run Running the ThreadedLoop() objects
 *
 * The run() methods will run the ThreadedLoop, invoking the specified
 * function or functor once per voxel in the loop. The different versions
 * will expect different signatures for the function or functor, and manage
 * looping over different numbers of `ImageType` classes. This should be
 * clarified by examining the examples below.
 *
 * This example can be used to compute the exponential of the voxel values
 * in-place, while displaying a progress message:
 *
 * ~~~{.cpp}
 * void my_function (MyImageType& vox) {
 *   vox.value() = std::exp (vox.value());
 * }
 *
 * ...
 *
 * MyImageType vox;
 * ThreadedLoop ("computing exponential in-place", vox)
 *   .run (my_function, vox);
 * ~~~
 *
 * To make this operation more easily generalisable to any `ImageType`, use a
 * functor with a template operator() method:
 *
 * ~~~{.cpp}
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
 * AnyImageType vox;
 * ThreadedLoop ("computing exponential in-place", vox)
 *   .run (MyFunction(), vox);
 * ~~~
 *
 * Note that a simple operation such as the previous example can be written
 * more compactly using C++11 lambda expressions:
 * ~~~{.cpp}
 * MyImageType vox;
 * ThreadedLoop ("computing exponential in-place", vox)
 *   .run ([](decltype(vox)& v) { v.value() = std::exp(v.value()); }, vox);
 * ~~~
 *
 *
 * As a further example, the following snippet performs the addition of any
 * `ImageTypes` \a vox1 and \a vox2, this time storing the results in \a
 * vox_out, with no progress display, and looping according to the strides
 * in \a vox1:
 * ~~~{.cpp}
 * struct MyAdd {
 *   template <class ImageType1, class ImageType2, class ImageType3>
 *     void operator() (ImageType1& out, const ImageType2& in1, const ImageType3& in2) {
 *       out.value() = in1.value() + in2.value();
 *     }
 * };
 *
 * ...
 *
 * ThreadedLoop (vox1).run (MyAdd(), vox_out, vox1, vox2);
 * ~~~
 *
 * Note that the `ImageType` arguments to run() must be provided in the same
 * order as expected by the Functor's `operator()` method.
 *
 * Again, such a simple operation can be written more compactly using C++11
 * lambda expressions:
 * ~~~{.cpp}
 * auto f = [](decltype(vox_out)& out, decltype(vox1)& in1, decltype(vox2)& in2) {
 *   out.value() = in1.value() + in2.value();
 * }
 * ThreadedLoop (vox1).run (f, vox_out, vox1, vox2);
 * ~~~
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
 * ThreadedLoop ("computing RMS of \"" + vox.name() + "\"", vox)
 *     .run (RMS(SoS), vox);
 *
 * double rms = std::sqrt (SoS / voxel_count (vox));
 * \endcode
 *
 * \section threaded_loop_run_outer The run_outer() method
 *
 * The run_outer() method can be used if needed to loop over the indices in
 * the outer loop only. This method is used internally by the run() methods,
 * and may prove useful in selected cases if each thread needs to handle its
 * own looping over the inner axes. Usage is essentially identical to the
 * run() method, and the function or functor provided will need to have a
 * void operator() (const Iterator& pos) method defined. Note that in this
 * case, any `ImageType` classes to be looped over will need to be stored as
 * members to the functor, since it will only receive an `Iterator` for each
 * invocation - the functor will need to then implement looping over the
 * inner axes from the position provided in the `Iterator`.
 *
 * \sa Loop
 * \sa Thread::run()
 * \sa thread_queue
 *
 * @}
 */

namespace {

inline std::vector<size_t> get_inner_axes(const std::vector<size_t> &axes, size_t num_inner_axes) {
  return {axes.begin(), axes.begin() + num_inner_axes};
}

inline std::vector<size_t> get_outer_axes(const std::vector<size_t> &axes, size_t num_inner_axes) {
  return {axes.begin() + num_inner_axes, axes.end()};
}

template <class HeaderType>
inline std::vector<size_t>
get_inner_axes(const HeaderType &source, size_t num_inner_axes, size_t from_axis, size_t to_axis) {
  return get_inner_axes(Stride::order(source, from_axis, to_axis), num_inner_axes);
}

template <class HeaderType>
inline std::vector<size_t>
get_outer_axes(const HeaderType &source, size_t num_inner_axes, size_t from_axis, size_t to_axis) {
  return get_outer_axes(Stride::order(source, from_axis, to_axis), num_inner_axes);
}

template <int N, class Functor, class... ImageType> struct ThreadedLoopRunInner {
  const std::vector<size_t> &outer_axes;
  decltype(Loop(outer_axes)) loop;
  typename std::remove_reference<Functor>::type func;
  std::tuple<ImageType...> vox;

  ThreadedLoopRunInner(const std::vector<size_t> &outer_axes,
                       const std::vector<size_t> &inner_axes,
                       const Functor &functor,
                       ImageType &...voxels)
      : outer_axes(outer_axes), loop(Loop(inner_axes)), func(functor), vox(voxels...) {}

  void operator()(const Iterator &pos) {
    assign_pos_of(pos, outer_axes).to(vox);
    for (auto i = std::apply(loop, vox); i; ++i)
      std::apply(func, vox);
  }
};

template <class Functor, class... ImageType> struct ThreadedLoopRunInner<0, Functor, ImageType...> {
  const std::vector<size_t> &outer_axes;
  decltype(Loop(outer_axes)) loop;
  typename std::remove_reference<Functor>::type func;

  ThreadedLoopRunInner(const std::vector<size_t> &outer_axes,
                       const std::vector<size_t> &inner_axes,
                       const Functor &functor,
                       ImageType &.../*voxels*/)
      : outer_axes(outer_axes), loop(Loop(inner_axes)), func(functor) {}

  void operator()(Iterator &pos) {
    for (auto i = loop(pos); i; ++i)
      func(pos);
  }
};

inline void __manage_progress(...) {}
template <class LoopType, class ThreadType>
inline auto __manage_progress(const LoopType *loop, const ThreadType *threads)
    -> decltype((void)(&loop->progress), void()) {
  loop->progress.run_update_thread(*threads);
}

template <class OuterLoopType> struct ThreadedLoopRunOuter {
  Iterator iterator;
  OuterLoopType outer_loop;
  std::vector<size_t> inner_axes;

  //! invoke \a functor (const Iterator& pos) per voxel <em> in the outer axes only</em>
  template <class Functor> void run_outer(Functor &&functor) {
    if (Thread::threads_to_execute() == 0) {
      for (auto i = outer_loop(iterator); i; ++i)
        functor(iterator);
      return;
    }

    ProgressBar::SwitchToMultiThreaded progress_functions;

    struct Shared {
      Shared(const Shared &) = delete;
      Shared(Shared &&) = delete;
      Shared &operator=(const Shared &) = delete;
      Shared &operator=(Shared &&) = delete;
      ~Shared() = default;
      Iterator &iterator;
      decltype(outer_loop(iterator)) loop;
      FORCE_INLINE bool next(Iterator &pos) {
        if (loop) {
          assign_pos_of(iterator, loop.axes).to(pos);
          ++loop;
          return true;
        } else
          return false;
      }
    };

    MutexProtected<Shared> shared = {iterator, outer_loop(iterator)};

    struct PerThread {
      MutexProtected<Shared> &shared;
      PerThread(const PerThread &) = default;
      PerThread(PerThread &&) noexcept = default;
      PerThread &operator=(const PerThread &) = delete;
      PerThread &operator=(PerThread &&) = delete;
      ~PerThread() = default;
      typename std::remove_reference<Functor>::type func;
      void execute() {
        auto pos = shared.lock()->iterator;
        while (shared.lock()->next(pos))
          func(pos);
      }
    } loop_thread = {shared, functor};

    auto threads = Thread::run(Thread::multi(loop_thread), "loop threads");

    auto *loop = &(shared.lock()->loop);
    __manage_progress(loop, &threads);
    threads.wait();
  }

  //! invoke \a functor (const Iterator& pos) per voxel <em> in the outer axes only</em>
  template <class Functor, class... ImageType> void run(Functor &&functor, ImageType &&...vox) {
    ThreadedLoopRunInner<sizeof...(ImageType),
                         typename std::remove_reference<Functor>::type,
                         typename std::remove_reference<ImageType>::type...>
    loop_thread(outer_loop.axes, inner_axes, functor, vox...);
    run_outer(loop_thread);
    check_app_exit_code();
  }
};
} // namespace

//! Multi-threaded loop object
//* \sa image_thread_looping for details */
template <class HeaderType>
inline ThreadedLoopRunOuter<decltype(Loop(std::vector<size_t>()))>
ThreadedLoop(const HeaderType &source, const std::vector<size_t> &outer_axes, const std::vector<size_t> &inner_axes) {
  return {source, Loop(outer_axes), inner_axes};
}

//! Multi-threaded loop object
//* \sa image_thread_looping for details */
template <class HeaderType>
inline ThreadedLoopRunOuter<decltype(Loop(std::vector<size_t>()))>
ThreadedLoop(const HeaderType &source, const std::vector<size_t> &axes, size_t num_inner_axes = 1) {
  return {source, Loop(get_outer_axes(axes, num_inner_axes)), get_inner_axes(axes, num_inner_axes)};
}

//! Multi-threaded loop object
//* \sa image_thread_looping for details */
template <class HeaderType>
inline ThreadedLoopRunOuter<decltype(Loop(std::vector<size_t>()))>
ThreadedLoop(const HeaderType &source,
             size_t from_axis = 0,
             size_t to_axis = std::numeric_limits<size_t>::max(),
             size_t num_inner_axes = 1) {
  return {source,
          Loop(get_outer_axes(source, num_inner_axes, from_axis, to_axis)),
          get_inner_axes(source, num_inner_axes, from_axis, to_axis)};
}

//! Multi-threaded loop object
//* \sa image_thread_looping for details */
template <class HeaderType>
inline ThreadedLoopRunOuter<decltype(Loop("", std::vector<size_t>()))>
ThreadedLoop(const std::string &progress_message,
             const HeaderType &source,
             const std::vector<size_t> &outer_axes,
             const std::vector<size_t> &inner_axes) {
  return {source, Loop(progress_message, outer_axes), inner_axes};
}

//! Multi-threaded loop object
//* \sa image_thread_looping for details */
template <class HeaderType>
inline ThreadedLoopRunOuter<decltype(Loop("", std::vector<size_t>()))> ThreadedLoop(const std::string &progress_message,
                                                                                    const HeaderType &source,
                                                                                    const std::vector<size_t> &axes,
                                                                                    size_t num_inner_axes = 1) {
  return {source, Loop(progress_message, get_outer_axes(axes, num_inner_axes)), get_inner_axes(axes, num_inner_axes)};
}

//! Multi-threaded loop object
//* \sa image_thread_looping for details */
template <class HeaderType>
inline ThreadedLoopRunOuter<decltype(Loop("", std::vector<size_t>()))>
ThreadedLoop(const std::string &progress_message,
             const HeaderType &source,
             size_t from_axis = 0,
             size_t to_axis = std::numeric_limits<size_t>::max(),
             size_t num_inner_axes = 1) {
  return {source,
          Loop(progress_message, get_outer_axes(source, num_inner_axes, from_axis, to_axis)),
          get_inner_axes(source, num_inner_axes, from_axis, to_axis)};
}

} // namespace MR

#endif
