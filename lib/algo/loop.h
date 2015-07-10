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

#ifndef __algo_loop_h__
#define __algo_loop_h__

#include "apply.h"
#include "progressbar.h"
#include "stride.h"
#include "image_helpers.h"

namespace MR
{

  //! \cond skip

  template <class LoopType, typename... ImageType>
    class LoopIter {
      private:
        struct Start {
          LoopType& loop;
          FORCE_INLINE void operator() (ImageType&... x) { loop.start (x...); }
        };
        struct Inc {
          LoopType& loop;
          FORCE_INLINE void operator() (ImageType&... x) { loop.next (x...); }
        };
      public:
        FORCE_INLINE LoopIter (const LoopType& loop_type, ImageType&... voxels) : 
          loop (loop_type), vox (voxels...) { 
            //unpack ([this](ImageType&... v) { loop.start (v...); }, vox); 
            unpack (Start ({ loop }), vox); 
          }
        FORCE_INLINE LoopIter (LoopIter&&) = default;

        LoopIter () = delete;
        LoopIter (const LoopIter&) = delete;
        LoopIter& operator=(const LoopIter&) = delete;

        FORCE_INLINE operator bool() const { return loop.ok(); }
        FORCE_INLINE void operator++ () { unpack (Inc ({ loop }), vox); }
        FORCE_INLINE void operator++ (int) { unpack (Inc ({ loop }), vox); }
      private:
        LoopType loop;
        std::tuple<ImageType&...> vox;
    };


  namespace {

    struct set_pos {
      FORCE_INLINE set_pos (size_t axis, ssize_t index) : axis (axis), index (index) { }
      template <class ImageType> 
        FORCE_INLINE void operator() (ImageType& vox) { vox.index(axis) = index; }
      size_t axis;
      ssize_t index;
    };

    struct inc_pos {
      FORCE_INLINE inc_pos (size_t axis) : axis (axis) { }
      template <class ImageType> 
        FORCE_INLINE void operator() (ImageType& vox) { ++vox.index(axis); }
      size_t axis;
    };

  }

  //! \endcond

  /** \defgroup loop Looping functions
    @{ */

  //! a class to loop over arbitrary numbers of axes of a ImageType  
  /*! This class can be used to loop over any number of axes of one of more
   * ImageType classes, within the same thread of execution (for
   * multi-threaded applications, see Image::ThreadedLoop). Its use is best
   * illustrated with the following examples.
   *
   * If \a vox in the following example is a 3D ImageType (i.e. vox.ndim() ==
   * 3), then:
   * \code
   * float sum = 0.0;
   * Image::Loop loop;
   * for (auto i = loop.run (vox); i; ++i)
   *   sum += vox.value();
   * \endcode
   * is equivalent to:
   * \code
   * float sum = 0.0;
   * for (vox.index(2) = 0; vox.index(2) < vox.size(2); ++vox.index(2))
   *   for (vox.index(1) = 0; vox.index(1) < vox.size(1); ++vox.index(1))
   *     for (vox.index(0) = 0; vox.index(0) < vox.size(0); ++vox.index(0))
   *       sum += vox.value();
   * \endcode
   * This has the advantage that the dimensionality of the ImageType does not
   * need to be known at compile-time. In other words, if the ImageType was
   * 4-dimensional, the first looping construct would correctly iterate over
   * all voxels, whereas the second one would only process the first 3D
   * volume.
   *
   * \section multiloop Looping over multiple ImageType objects
   *
   * It is often required to loop over more than one ImageType of the same
   * dimensions. This is done trivially by passing any additional ImageType
   * objects to be looped over to the run() member function. For example,
   * this code snippet will copy the contents of the ImageType \a src into a
   * ImageType \a dest, assumed to have the same dimensions as \a src:
   * \code
   * for (auto i = Image::Loop().run(dest, src); i; ++i)
   *   dest.value() = vox.value();
   * \endcode
   *
   * \section restrictedloop Looping over a specific range of axes
   * It is also possible to explicitly specify the range of axes to be looped
   * over. In the following example, the program will loop over each 3D
   * volume in the ImageType in turn:
   * \code
   * Image::Loop outer (3); // outer loop iterates over axis 3
   * for (auto i = outer.run (vox)); i; ++i {
   *   float sum = 0.0;
   *   Image::Loop inner (0, 3); // inner loop iterates over axes 0 to 2
   *   for (auto j = inner.run (vox); j; ++j)
   *     sum += vox.value();
   *   print ("total = " + str (sum) + "\n");
   * }
   * \endcode
   *
   * \section progressloop Displaying progress status
   *
   * The Loop object can also display its progress as it proceeds, using the
   * appropriate constructor. In the following example, the program will
   * display its progress as it averages a ImageType:
   * \code
   * float sum = 0.0;
   *
   * Loop loop ("averaging...");
   * for (auto i = loop.run (vox); i; ++i) 
   *   sum += vox.value();
   *
   * float average = sum / float (Image::voxel_count (vox));
   * print ("average = " + str (average) + "\n");
   * \endcode
   * The output would look something like this:
   * \code
  * myprogram: averaging... 100%
    * average = 23.42
    * \endcode
    *
    * \sa Image::LoopInOrder
    * \sa Image::ThreadedLoop
    */
    class Loop
    {
      public:

        //! Constructor
        /*! Construct a Loop object to iterate over the axes specified. With
         * no arguments, the Loop will iterate over all axes of the first ImageType
         * supplied to next(). If a single argument is specified, the Loop will
         * iterate over that axis only. If both \a from_axis and \a to_axis are specified,
         * the Loop will iterate from axis \a from_axis up to but \b not
         * including axis \a to_axis. */
        FORCE_INLINE explicit Loop (size_t from_axis, size_t to_axis) :
          from_ (from_axis), to_ (to_axis), max_axis_ (0), cont_ (true) { }

        //! \copydoc Loop(size_t,size_t)
        FORCE_INLINE explicit Loop (size_t axis) : Loop (axis, axis+1) { }
        //! \copydoc Loop(size_t,size_t)
        FORCE_INLINE explicit Loop () : Loop (0, std::numeric_limits<size_t>::max()) { }

        //! Constructor with progress status
        /*! Construct a Loop object to iterate over the axes specified and
         * display the progress status with the specified message. With no
         * arguments, the Loop will iterate over all axes of the first
         * ImageType supplied to next(). If a single argument is specified, the
         * Loop will iterate over that axis only. If both \a from_axis and \a
         * to_axis are specified, the Loop will iterate from axis \a from_axis
         * up to but \b not including axis \a to_axis. */
        FORCE_INLINE explicit Loop (const std::string& message, size_t from_axis, size_t to_axis) :
          from_ (from_axis), to_ (to_axis), max_axis_ (0), cont_ (true), progress_ (message, 1) { }
        
        //! \copydoc Loop(const std::string&,size_t,size_t)
        FORCE_INLINE explicit Loop (const std::string& message, size_t axis) : Loop (message, axis, axis+1) { }
        //! \copydoc Loop(const std::string&,size_t,size_t)
        FORCE_INLINE explicit Loop (const std::string& message) : Loop (message, 0, std::numeric_limits<size_t>::max()) { }

        //! return iteratable object for use in loop
        /*! This start the loop by resetting the appropriate coordinates of
         * each of the specified ImageType objects to zero, and initialising
         * the progress status if appropriate. Note that only those axes
         * specified in the Loop constructor will have their coordinates set
         * to zero; the coordinates of all other axes will be left untouched.
         *
         * The object returned by this function is designed to be used
         * directly the loop, for example:
         * \code
         * Loop loop ("copy...");
         * for (auto i = loop (vox_in, vox_out); i; ++i)
         *   vox_out.value() = vox_in.value();
         * \endcode */
        template <typename... ImageType> 
          FORCE_INLINE LoopIter<Loop,ImageType&...> run (ImageType&... vox) {
            return { *this, vox... };
          }
        //! equivalent to run()
        template <typename... ImageType> 
          FORCE_INLINE LoopIter<Loop,ImageType&...> operator() (ImageType&... vox) {
            return { *this, vox... };
          }

        //! Start the loop to iterate over the VoxelTypes specified
        /*! Start the loop by resetting the appropriate coordinates of each of
         * the specified ImageType objects to zero, and initialising the progress status
         * if appropriate. Note that only those axes specified in the Loop
         * constructor will have their coordinates set to zero; the coordinates
         * of all other axes will be left untouched. */
        template <typename... ImageType>
          FORCE_INLINE void start (ImageType&... vox) {
            cont_ = true;
            auto& ref = std::get<0> (std::tie (vox...));
            max_axis_ = std::min (ref.ndim(), to_);
            first_axis_dim = ref.size (from_) - 1;
            for (size_t n = from_; n < max_axis_; ++n) 
              apply (set_pos (n, 0), std::tie (vox...));

            if (progress_)
              progress_.set_max (voxel_count (ref, from_, to_));
          }

        //! Check whether the loop should continue iterating
        /*! \return true if the loop has not completed, false otherwise. */
        FORCE_INLINE bool ok() const {
          return cont_;
        }

        //! Advance coordinates of specified VoxelTypes
        /*! Advance coordinates of all specified ImageType objects to the next position
         * to be processed, and update the progress status if appropriate. */
        template <typename... ImageType>
          FORCE_INLINE void next (ImageType&... vox) {
            auto tvox = std::tie (vox...);
            auto& ref = std::get<0> (tvox);
            ++progress_;
            if (ref.index (from_) < first_axis_dim)
              apply (inc_pos (from_), tvox);
            else {
              next_axis (from_+1, tvox);
              if (cont_) 
                apply (set_pos (from_, 0), tvox);
            }
          }    
        

        //! set position along relevant axes of \a target to that of \a reference
        /*! set the position of \a target along those axes involved in the
         * loop to the that of \a reference, leaving all other coordinates
         * unchanged. */
        template <typename RefImageType, typename... ImageType>
          FORCE_INLINE void set_position (const RefImageType& reference, ImageType&... target) const {
            auto t = std::tie (target...);
            set_position (reference, t);
          }

        //! set position along relevant axes of \a target to that of \a reference
        /*! set the position of \a target along those axes involved in the
         * loop to the that of \a reference, leaving all other coordinates
         * unchanged. */
        template <typename RefImageType, typename... ImageType>
          FORCE_INLINE void set_position (const RefImageType& reference, std::tuple<ImageType&...>& target) const {
            for (size_t i = from_; i < max_axis_; ++i) 
              apply (set_pos (i, reference.index(i)), target);
          }


      private:
        const size_t from_, to_;
        size_t max_axis_;
        ssize_t first_axis_dim;
        bool cont_;
        ProgressBar progress_;
        
        template <typename... ImageType> 
          void next_axis (size_t axis, const std::tuple<ImageType&...>& vox) {
            if (axis < max_axis_) {
              if (std::get<0>(vox).index(axis) + 1 < std::get<0>(vox).size (axis)) 
                apply (inc_pos (axis), vox);
              else {
                if (axis+1 == max_axis_) {
                  cont_ = false;
                  progress_.done();
                }
                else {
                  next_axis (axis+1, vox);
                  if (cont_) 
                    apply (set_pos (axis, 0), vox);
                }
              }
            }
            else {
              cont_ = false;
              progress_.done();
            }
          }
    };








  //! a class to loop over arbitrary numbers and orders of axes of a ImageType
  /*! This class can be used to loop over any number of axes of one of more
   * ImageType, in any specified order, within the same thread of execution
   * (for multi-threaded applications, see Image::ThreadedLoop). Its use is
   * essentially identical to that of the Loop class, with the difference
   * that axes can now be iterated over in any arbitrary order. This is best
   * illustrated with the following examples.
   *
   * \section strideorderloop Looping with smallest stride first
   * The looping strategy most likely to make most efficient use of the
   * memory infrastructure is one where the innermost loop iterates over the
   * axis with the smallest absolute stride, since voxels along this axis are
   * most likely to be adjacent. This is most likely to optimise both
   * throughput to and from system RAM or disk (which are typically optimised
   * for bursts of contiguous sections of memory), and CPU cache usage.
   *
   * The LoopInOrder class is designed to facilitate this. In the following
   * example, the ImageType of interest is passed as an argument to the
   * constructor, so that its strides can be used to compute the nesting
   * order for the loops over the corresponding axes. Here, we assume that
   * \a vox is a 3D ImageType (i.e. vox.ndim() == 3) with strides [ 2 -1 3 ]:
   * \code
   * float sum = 0.0;
   * for (auto i = Image::LoopInOrder().run (vox); i; ++i)
   *   sum += vox.value();
   * \endcode
   * This is equivalent to:
   * \code
   * float sum = 0.0;
   * for (vox.index(2) = 0; vox.index(2) < vox.size(2); ++vox.index(2))
   *   for (vox.index(0) = 0; vox.index(0) < vox.size(0); ++vox.index(0))
   *     for (vox.index(1) = 0; vox.index(1) < vox.size(1); ++vox.index(1))
   *       sum += vox.value();
   * \endcode
   *
   * \section restrictedorderloop Looping over a specific range of axes
   * It is also possible to explicitly specify the range of axes to be looped
   * over. In the following example, the program will loop over each 3D
   * volume in the ImageType in turn using the Loop class, and use the
   * LoopInOrder class to iterate over the axes of each volume to ensure
   * efficient memory bandwidth use when each volume is being processed.
   * \code
   * // define inner loop to iterate over axes 0 to 2
   * LoopInOrder inner (vox, 0, 3);
   *
   * // outer loop iterates over axes 3 and above:
   * for (auto i = Loop(3).run (vox); i; ++i) {
   *   float sum = 0.0;
   *   for (auto j = inner.run (vox); j; ++j) {
   *     sum += vox.value();
   *   print ("total = " + str (sum) + "\n");
   * }
   * \endcode
   *
   * \section arbitraryorderloop Arbitrary order loop
   * It is also possible to specify the looping order explictly, as in the
   * following example:
   * \code
   * float value = 0.0;
   * std::vector<size_t> order = { 1, 0, 2 };
   *
   * LoopInOrder loop (vox, order);
   * for (auto i = loop.run (vox); i; ++i) 
   *   value += std::exp (-vox.value());
   * \endcode
   * This will iterate over the axes in the same order as the first example
   * above, irrespective of the strides of the ImageType.
   *
   * \section multiorderloop Looping over multiple ImageType objects:
   *
   * As with the Loop class, it is possible to loop over more than one
   * ImageType of the same dimensions, by passing any additional ImageType
   * objects to be looped over to the run() member function. For example,
   * this code snippet will copy the contents of the ImageType \a src into a
   * ImageType \a dest (assumed to have the same dimensions as \a src),
   * with the looping order optimised for the \a src ImageType:
   * \code
   * LoopInOrder loop (src);
   * for (auto i = loop.run(src, dest); i; ++i) 
    *   dest.value() = src.value();
    * \endcode
    *
    * \section progressloopinroder Displaying progress status
    * As in the Loop class, the LoopInOrder object can also display its
    * progress as it proceeds, using the appropriate constructor. In the
    * following example, the program will display its progress as it averages
    * a ImageType:
    * \code
    * float sum = 0.0;
  *
    * LoopInOrder loop (vox, "averaging...");
  * for (auto i = loop.run (vox); i; ++i)
    *   sum += vox.value();
  *
    * float average = sum / float (Image::voxel_count (vox));
  * print ("average = " + str (average) + "\n");
  * \endcode
    * The output would look something like this:
    * \code
    * myprogram: averaging... 100%
    * average = 23.42
    * \endcode
    *
    * \sa Image::Loop
    * \sa Image::ThreadedLoop
    */
    class LoopInOrder
    {
      public:
        //! Constructor from axes indices
        /*! Construct a LoopInOrder object to iterate over the axes specified. */
        FORCE_INLINE explicit LoopInOrder (const std::vector<size_t>& axes) :
          axes_ (axes), first_axis (axes_[0]), cont_ (true) { }

        //! Construct from axes indices with progress status
        /*! Construct a LoopInOrder object to iterate over the axes specified and
         * display the progress status with the specified message. */
        FORCE_INLINE explicit LoopInOrder (const std::string& message, const std::vector<size_t>& axes) :
          axes_ (axes), first_axis (axes_[0]), cont_ (true), progress_ (message, 1) { }

        //! Construct from ImageType strides
        /*! Construct a LoopInOrder object to iterate over the axes of \a image
         * in order of smallest stride first. With no arguments, the Loop will
         * iterate over all axes of \a image. If both \a from_axis and \a
         * to_axis are specified, the Loop will iterate from \a from_axis up to
         * but \b not including \a to_axis. 
         *
         * \note If only \a from_axis is specified, \a to_axis defaults to all
         * remaining axes above \a from_axis - looping will \b not be restricted
         * to \a from_axis alone. Use Loop if you need to loop over a single
         * axis.*/
        template <class ImageType>
          FORCE_INLINE explicit LoopInOrder (const ImageType& vox, size_t from_axis = 0, size_t to_axis = std::numeric_limits<size_t>::max()) :
            axes_ (Stride::order (vox, from_axis, to_axis)), first_axis (axes_[0]), cont_ (true) { }

        //! Constructor from ImageType strides with progress status
        /*! Construct a LoopInOrder object to iterate over the axes of \a image
         * in order of smallest stride first, and display the progress status                                                                                                                                  
         * with the specified message. With no arguments, the Loop will
         * iterate over all axes of \a image. If both \a from_axis and \a
         * to_axis are specified, the Loop will iterate from \a from_axis up to
         * but \b not including \a to_axis. 
         *
         * \note If only \a from_axis is specified, \a to_axis defaults to all
         * remaining axes above \a from_axis - looping will \b not be restricted
         * to \a from_axis alone. Use Loop if you need to loop over a single
         * axis.*/
        template <class ImageType>
          FORCE_INLINE explicit LoopInOrder (const std::string& message, const ImageType& vox, 
              size_t from_axis = 0, size_t to_axis = std::numeric_limits<size_t>::max()) :
            axes_ (Stride::order (vox, from_axis, to_axis)), first_axis (axes_[0]), cont_ (true), progress_ (message, 1) { }

        //! return iteratable object for use in loop
        /*! This start the loop by resetting the appropriate coordinates of
         * each of the specified ImageType objects to zero, and initialising
         * the progress status if appropriate. Note that only those axes
         * specified in the Loop constructor will have their coordinates set
         * to zero; the coordinates of all other axes will be left untouched.
         *
         * The object returned by this function is designed to be used
         * directly the loop, for example:
         * \code
         * LoopInOrder loop (vox_in, "copy...");
         * for (auto i = loop (vox_in, vox_out); i; ++i)
         *   vox_out.value() = vox_in.value();
         * \endcode */
        template <typename... ImageType> 
          FORCE_INLINE LoopIter<LoopInOrder,ImageType&...> run (ImageType&... vox) {
            return { *this, vox... };
          }

        //! equivalent to run()
        template <typename... ImageType> 
          FORCE_INLINE LoopIter<LoopInOrder,ImageType&...> operator() (ImageType&... vox) {
            return { *this, vox... };
          }


        //! Start the loop to iterate over the VoxelTypes specified
        /*! Start the loop by resetting the appropriate coordinates of each of
         * the specified ImageType objects to zero, and initialising the progress status
         * if appropriate. Note that only those axes specified in the Loop
         * constructor will have their coordinates set to zero; the coordinates
         * of all other axes will be left untouched. */
        template <typename... ImageType>
          FORCE_INLINE void start (ImageType&... vox) {
            cont_ = true;
            auto& ref = std::get<0> (std::tie (vox...));
            first_axis_dim = ref.size (first_axis) - 1;
            for (size_t n = 0; n < axes_.size(); ++n) 
              apply (set_pos (axes_[n], 0), std::tie (vox...));
            if (progress_)
              progress_.set_max (voxel_count (ref, axes_));
          }

        //! Check whether the loop should continue iterating
        /*! \return true if the loop has not completed, false otherwise. */
        FORCE_INLINE bool ok() const {
          return cont_;
        }

        //! Advance coordinates of specified VoxelTypes
        /*! Advance coordinates of all specified ImageType objects to the next position
         * to be processed, and update the progress status if appropriate. */
        template <typename... ImageType>
          FORCE_INLINE void next (ImageType&... vox) {
            auto tvox = std::tie (vox...);
            auto& ref = std::get<0> (tvox);
            ++progress_;
            if (ref.index(first_axis) < first_axis_dim)
              apply (inc_pos (first_axis), tvox);
            else {
              next_axis (1, tvox);
              if (cont_) 
                apply (set_pos (first_axis, 0), tvox);
            }
          }

        //! set position along relevant axes of \a target to that of \a reference
        /*! set the position of \a target along those axes involved in the
         * loop to the that of \a reference, leaving all other coordinates
         * unchanged. */
        template <typename RefImageType, typename... ImageType>
          FORCE_INLINE void set_position (const RefImageType& reference, ImageType&... target) const {
            auto t = std::tie (target...);
            set_position (reference, t);
          }

        //! set position along relevant axes of \a target to that of \a reference
        /*! set the position of \a target along those axes involved in the
         * loop to the that of \a reference, leaving all other coordinates
         * unchanged. */
        template <typename RefImageType, typename... ImageType>
          FORCE_INLINE void set_position (const RefImageType& reference, std::tuple<ImageType&...>& target) const {
            for (size_t i = 0; i < axes_.size(); ++i) 
              apply (set_pos (axes_[i], reference.index(axes_[i])), target);
          }


        FORCE_INLINE const std::vector<size_t>& axes () const { return axes_; }

      private:
        const std::vector<size_t> axes_;
        const size_t first_axis;
        ssize_t first_axis_dim;
        bool cont_;
        ProgressBar progress_;

        template <typename... ImageType>
          void next_axis (size_t axis, std::tuple<ImageType&...>& vox) {
            if (axis == axes_.size()) {
              cont_ = false;
              progress_.done();
              return;
            }

            size_t a = axes_[axis];
            if (std::get<0>(vox).index(a) < std::get<0>(vox).size (a) - 1) 
              apply (inc_pos (a), vox);
            else {
              next_axis (axis+1, vox);
              if (cont_) apply (set_pos (a, 0), vox);
            }
          }

    };


  namespace {

    template <class... ImageType> struct  __LoopImagesAlongAxisRange {
      const std::tuple<ImageType&...> vox;
      const size_t from, to;

      template <class Functor> void run (const std::string& progress_message, Functor&& body) const {
        ProgressBar progress (progress_message, voxel_count (std::get<0>(vox), from, to));
        run ([&]() { body(); ++progress; });
      }

      template <class Functor> void run (Functor&& body) const {
        auto& ref = std::get<0>(vox);
        const ssize_t size0 = ref.size(from);
        for (size_t n = from+1; n < to; ++n)
          apply (set_pos ({ n, 0 }), vox);

next_iteration:
        for (apply (set_pos ({ from, 0 }), vox); ref.index(from) < size0; apply (inc_pos ({ from }), vox))
          body();

        size_t axis = from+1;
inc_next_axis:
        apply (inc_pos ({ axis }), vox);
        if (ref.index(axis) < ref.size(axis))
          goto next_iteration;
        apply (set_pos ({ axis, 0 }), vox);
        ++axis;
        if (axis < to)
          goto inc_next_axis;
      }
    };


    template <class... ImageType> struct  __LoopImagesAlongSingleAxis {
      const std::tuple<ImageType&...> vox;
      const size_t axis;

      template <class Functor> void run (const std::string& progress_message, Functor&& body) const {
        ProgressBar progress (progress_message, std::get<0>(vox).size(axis));
        run ([&]() { body(); ++progress; });
      }

      template <class Functor> FORCE_INLINE void run (Functor&& body) const {
        auto& ref = std::get<0>(vox);
        const ssize_t size0 = ref.size(axis);
        for (apply (set_pos ({ axis, 0 }), vox); ref.index(axis) < size0; apply (inc_pos ({ axis }), vox))
          body();
      }
      FORCE_INLINE const __LoopImagesAlongAxisRange<ImageType&...> to (const size_t axis_to) const { return { vox, axis, axis_to }; }
    };




    template <class... ImageType> struct  __LoopImagesAlongStaticAxes {
      const std::tuple<ImageType&...> vox;
      const std::initializer_list<size_t> axes;

      template <class Functor> void run (const std::string& progress_message, Functor&& body) const {
        ProgressBar progress (progress_message, voxel_count (std::get<0>(vox), axes));
        run ([&]() { body(); ++progress; });
      }

      template <class Functor> void run (Functor&& body) const {
        auto& ref = std::get<0>(vox);
        const size_t from = *axes.begin();
        const ssize_t size0 = ref.size(from);
        for (auto axis = axes.begin()+1; axis != axes.end(); ++axis)
          apply (set_pos ({ *axis, 0 }), vox);

next_iteration:
        for (apply (set_pos ({ from, 0 }), vox); ref.index(from) < size0; apply (inc_pos ({ from }), vox))
          body();

        auto axis = axes.begin()+1;
inc_next_axis:
        apply (inc_pos ({ *axis }), vox);
        if (ref.index(*axis) < ref.size(*axis))
          goto next_iteration;
        apply (set_pos ({ *axis, 0 }), vox);
        ++axis;
        if (axis != axes.end())
          goto inc_next_axis;
      }
    };



    template <class... ImageType> struct  __LoopImagesAlongDynamicAxes {
      const std::tuple<ImageType&...> vox;
      const std::vector<size_t> axes;

      template <class Functor> void run (const std::string& progress_message, Functor&& body) const {
        ProgressBar progress (progress_message, voxel_count (std::get<0>(vox), axes));
        run ([&]() { body(); ++progress; });
      }


      template <class Functor> void run (Functor&& body) const {
        auto& ref = std::get<0>(vox);
        const size_t from = axes[0];
        const ssize_t size0 = ref.size(from);
        size_t other_axes[axes.size()-1];
        size_t* last = other_axes;
        for (auto a = axes.cbegin()+1; a != axes.cend(); ++a)
          *last++ = *a;

        for (const auto axis : other_axes)
          apply (set_pos ({ axis, 0 }), vox);

next_iteration:
        for (apply (set_pos ({ from, 0 }), vox); ref.index(from) < size0; apply (inc_pos ({ from }), vox))
          body();

        auto axis = other_axes;
inc_next_axis:
        apply (inc_pos ({ *axis }), vox);
        if (ref.index(*axis) < ref.size(*axis))
          goto next_iteration;
        apply (set_pos ({ *axis, 0 }), vox);
        ++axis;
        if (axis != last)
          goto inc_next_axis;
      }
    };



    template <class... ImageType> struct __LoopImages {
      std::tuple<ImageType&...> vox;
      __LoopImages (ImageType&... images) : vox (images...) { }
      FORCE_INLINE const __LoopImagesAlongSingleAxis<ImageType&...> along (const size_t axis) const { return { vox, axis }; }
      FORCE_INLINE const __LoopImagesAlongStaticAxes<ImageType&...> along (const std::initializer_list<size_t> axes) const { return { vox, axes }; }
      FORCE_INLINE const __LoopImagesAlongDynamicAxes<ImageType&...> along (const std::vector<size_t>& axes) const { return { vox, axes }; }
      template <class RefImageType>
        FORCE_INLINE const __LoopImagesAlongDynamicAxes<ImageType&...> 
        according_to (const RefImageType& ref, size_t from_axis = 0, size_t to_axis = std::numeric_limits<size_t>::max()) const { return { vox, Stride::order (ref, from_axis, to_axis) }; }


      template <class Functor> void run (const std::string& progress_message, Functor&& body) const {
        according_to (std::get<0>(vox)).run (progress_message, body); 
      }
      template <class Functor> void run (Functor&& body) const { according_to (std::get<0>(vox)).run (body); }

    };



  }



  //! single-threaded looping over images
  /*!
   * This function and its associated classes allow looping over arbitrary
   * images, along specified axes, and optionally with a progress report.
   * It can be used as follows:
   *
   * The loop_over() function itself takes the images to be looped over as
   * arguments. It returns a proxy object with a number of methods, used to
   * specify the axes of the loop. These in turn return a further proxy object
   * with a run() method, specifying body of the loop in the form of a functor
   * or lambda function (with the lambda function typically being the most
   * convenient).
   *
   * For example:
   *
   * \par Looping over whole images
   * This loops over all voxels of the input images, writing the difference
   * between \a in1 & \a in2 into \a out. Note that this assumes all images
   * have the same dimensions - no checking is done in this function. Also, the
   * axes will be looped over in order of increasing (absolute) stride of the
   * first image specified (\a in1 in this example). Note the use of a lambda
   * function to provide the body of the loop (and the use of the [&] specifier
   * to make variables in the current scope available to the body of the loop:
   * \code
   * loop_over (in1, in2, out).run ([&]() { out.value() = in1.value() - in2.value(); });
   *
   * \par Looping over a single axis
   * This loops over two images \a input & \a output, copying input to output
   * along axis 3 (volume axis) for the current voxel. As before, we use a
   * lambda function to provide the body of the loop:
   * \code 
   * loop_over (input, output).along (3).run ([&]() { output.value() = input.value(); });
   * \endcode 
   *
   * \par Looping over a range of axes:
   * This loops over a single image, squaring the voxel intensities along the
   * spatial axes (0 to 3 - axis 3 is \e not included). In this example, we use
   * a functor to provide the body of the loop:
   * \code
   * struct Square {
   *   Image<float>& vox;
   *   void operator() () { vox.value() = math::pow2 (vox.value()); }
   * };
   *
   * loop_over (image).along (0).to (3).run (Square ({ image }));
   * \endcode
   *
   * \par Looping over selected axes known at compile time
   * This loops over the x & z axes, computing the exponential of its input
   * in-place:
   * \code
   * loop_over (input).over ({ 0, 2 }).run ([&]() { input.value() = std::exp (input.value()); });
   * \endcode
   *
   *
   * \par Looping over selected axes unknown at compile time
   * When the axes to be looped over are determined at runtime, they can be
   * passed as a std::vector:
   * \code 
   * std::vector axes = { 3,0,1,2 };
   * loop_over (input, output).over (axes).run ([&]() { output.value() -= input.value(); });
   * \endcode
   *
   * \par Looping over axes in order of increasing stride of a specific image
   * This will loop over images \a in & \a out, with the axes and order of
   * traversal determined by the order of increasing strides of \a ref, along
   * all non-spatial axes (greater than 3):
   * \code
   * loop_over (in, out).according_to (ref, 3).run ([&]() { out.value() *= in.value(); });
   * \endcode
   */
  /*
   * \par Looping with a progress report
   * All of the above examples can be modified to provide a progress report on
   * the command line by providing the text to be displayed as first argument
   * in the run() method:
   * \code
   * double rms = 0;
   * loop_over (in).run ("computing RMS...", [&]() { rms += Math::pow2 (in.value()); });
   * rms = std::sqrt (rms);
   * \endcode
   */
  template <class... ImageType>
    FORCE_INLINE const __LoopImages<ImageType&...> loop_over (ImageType&... images) { return { images... }; }

  //! @}
}

#endif


