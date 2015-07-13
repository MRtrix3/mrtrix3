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



  struct LoopAlongSingleAxis {
    const size_t axis;

    template <class... ImageType>
      struct Run {
        const size_t axis;
        const std::tuple<ImageType&...> vox;
        const ssize_t size0;
        FORCE_INLINE Run (const size_t axis, const std::tuple<ImageType&...>& vox) : 
          axis (axis), vox (vox), size0 (std::get<0>(vox).size(axis)) { apply (set_pos (axis, 0), vox); }
        FORCE_INLINE operator bool() const { return std::get<0>(vox).index(axis) < size0; }
        FORCE_INLINE void operator++() const { apply (inc_pos (axis), vox); }
        FORCE_INLINE void operator++(int) const { operator++(); }
      };

    template <class... ImageType>
      FORCE_INLINE Run<ImageType...> operator() (ImageType&... images) const { return { axis, std::tie (images...) }; }
  };

  struct LoopAlongSingleAxisProgress {
    const std::string& text;
    const size_t axis;

    template <class... ImageType>
      struct Run {
        MR::ProgressBar progress;
        const size_t axis;
        const std::tuple<ImageType&...> vox;
        const ssize_t size0;
        FORCE_INLINE Run (const std::string& text, const size_t axis, const std::tuple<ImageType&...>& vox) : 
          progress (text, std::get<0>(vox).size(axis)), axis (axis), vox (vox), size0 (std::get<0>(vox).size(axis)) { apply (set_pos (axis, 0), vox); }
        FORCE_INLINE operator bool() const { return std::get<0>(vox).index(axis) < size0; }
        FORCE_INLINE void operator++() { apply (inc_pos (axis), vox); ++progress; }
        FORCE_INLINE void operator++(int) { operator++(); }
      };

    template <class... ImageType>
      FORCE_INLINE Run<ImageType...> operator() (ImageType&... images) const { return { text, axis, std::tie (images...) }; }
  };



  struct LoopAlongAxisRange {
    const size_t from, to;

    template <class... ImageType>
      struct Run {
        const size_t from, to;
        const std::tuple<ImageType&...> vox;
        const ssize_t size0;
        bool ok;
        FORCE_INLINE Run (const size_t axis_from, const size_t axis_to, const std::tuple<ImageType&...>& vox) : 
          from (axis_from), to (axis_to ? axis_to : std::get<0>(vox).ndim()), vox (vox), size0 (std::get<0>(vox).size(from)), ok (true) { 
            for (size_t n = from; n < to; ++n)
              apply (set_pos (n, 0), vox); 
          }
        FORCE_INLINE operator bool() const { return ok; }
        FORCE_INLINE void operator++() { 
          apply (inc_pos (from), vox); 
          if (std::get<0>(vox).index(from) < size0)
            return;

          apply (set_pos (from, 0), vox);
          size_t axis = from+1;
          do {
            apply (inc_pos ({ axis }), vox);
            if (std::get<0>(vox).index(axis) < std::get<0>(vox).size(axis))
              return;
            apply (set_pos (axis, 0), vox);
            ++axis;
          } while (axis < to);
          ok = false;
        }
        FORCE_INLINE void operator++(int) { operator++(); }
      };

    template <class... ImageType>
      FORCE_INLINE Run<ImageType...> operator() (ImageType&... images) const { return { from, to, std::tie (images...) }; }
  };

  struct LoopAlongAxisRangeProgress : public LoopAlongAxisRange {
    const std::string& text;
    LoopAlongAxisRangeProgress (const std::string& text, const size_t from, const size_t to) :
      LoopAlongAxisRange ({ from, to }), text (text) { }

    template <class... ImageType>
      struct Run : public LoopAlongAxisRange::Run<ImageType...> {
        MR::ProgressBar progress;
        FORCE_INLINE Run (const std::string& text, const size_t from, const size_t to, const std::tuple<ImageType&...>& vox) : 
          LoopAlongAxisRange::Run<ImageType...> (from, to, vox), progress (text, MR::voxel_count (std::get<0>(vox), from, to)) { }
        FORCE_INLINE void operator++() { LoopAlongAxisRange::Run<ImageType...>::operator++(); ++progress; }
        FORCE_INLINE void operator++(int) { operator++(); }
      };

    template <class... ImageType>
      FORCE_INLINE Run<ImageType...> operator() (ImageType&... images) const { return { text, from, to, std::tie (images...) }; }
  };



  struct LoopAlongAxes {
    template <class... ImageType>
      FORCE_INLINE LoopAlongAxisRange::Run<ImageType...> operator() (ImageType&... images) const { return { 0, std::get<0>(std::tie(images...)).ndim(), std::tie (images...) }; }
  };

  struct LoopAlongAxesProgress {
    const std::string& text;
    template <class... ImageType>
      FORCE_INLINE LoopAlongAxisRangeProgress::Run<ImageType...> operator() (ImageType&... images) const { return { text, 0, std::get<0>(std::tie(images...)).ndim(), std::tie (images...) }; }
  };



  struct LoopAlongStaticAxes {
    const std::initializer_list<size_t> axes;

    template <class... ImageType>
      struct Run {
        const std::initializer_list<size_t> axes;
        const std::tuple<ImageType&...> vox;
        const size_t from;
        const ssize_t size0;
        bool ok;
        FORCE_INLINE Run (const std::initializer_list<size_t> axes, const std::tuple<ImageType&...>& vox) : 
          axes (axes), vox (vox), from (*axes.begin()), size0 (std::get<0>(vox).size(from)), ok (true) { 
            for (auto axis : axes)
              apply (set_pos (axis, 0), vox); 
          }
        FORCE_INLINE operator bool() const { return ok; }
        FORCE_INLINE void operator++() { 
          apply (inc_pos (from), vox); 
          if (std::get<0>(vox).index(from) < size0)
            return;

          apply (set_pos (from, 0), vox);
          auto axis = axes.begin()+1;
          do {
            apply (inc_pos ({ *axis }), vox);
            if (std::get<0>(vox).index(*axis) < std::get<0>(vox).size(*axis))
              return;
            apply (set_pos (*axis, 0), vox);
            ++axis;
          } while (axis != axes.end());
          ok = false;
        }
        FORCE_INLINE void operator++(int) { operator++(); }
      };

    template <class... ImageType>
      FORCE_INLINE Run<ImageType...> operator() (ImageType&... images) const { return { axes, std::tie (images...) }; }
  };

  struct LoopAlongStaticAxesProgress : public LoopAlongStaticAxes {
    const std::string& text;
    LoopAlongStaticAxesProgress (const std::string& text, const std::initializer_list<size_t> axes) : 
      LoopAlongStaticAxes ({ axes }), text (text) { }

    template <class... ImageType>
      struct Run : public LoopAlongStaticAxes::Run<ImageType...> {
        MR::ProgressBar progress;
        FORCE_INLINE Run (const std::string& text, const std::initializer_list<size_t> axes, const std::tuple<ImageType&...>& vox) : 
          LoopAlongStaticAxes::Run<ImageType...> (axes, vox), progress (text, MR::voxel_count (std::get<0>(vox), axes)) { }
        FORCE_INLINE void operator++() { LoopAlongStaticAxes::Run<ImageType...>::operator++(); ++progress; }
        FORCE_INLINE void operator++(int) { operator++(); }
      };

    template <class... ImageType>
      FORCE_INLINE Run<ImageType...> operator() (ImageType&... images) const { return { text, axes, std::tie (images...) }; }
  };



  struct LoopAlongDynamicAxes {
    const std::vector<size_t> axes;

    template <class... ImageType>
      struct Run {
        const std::vector<size_t>& axes;
        const std::tuple<ImageType&...> vox;
        const size_t from;
        const ssize_t size0;
        bool ok;
        FORCE_INLINE Run (const std::vector<size_t>& axes, const std::tuple<ImageType&...>& vox) : 
          axes (axes), vox (vox), from (axes[0]), size0 (std::get<0>(vox).size(from)), ok (true) { 
            for (auto axis : axes)
              apply (set_pos (axis, 0), vox); 
          }
        FORCE_INLINE operator bool() const { return ok; }
        FORCE_INLINE void operator++() { 
          apply (inc_pos (from), vox); 
          if (std::get<0>(vox).index(from) < size0)
            return;

          apply (set_pos (from, 0), vox);
          auto axis = axes.cbegin()+1;
          do {
            apply (inc_pos ({ *axis }), vox);
            if (std::get<0>(vox).index(*axis) < std::get<0>(vox).size(*axis))
              return;
            apply (set_pos (*axis, 0), vox);
            ++axis;
          } while (axis != axes.cend());
          ok = false;
        }
        FORCE_INLINE void operator++(int) { operator++(); }
      };

    template <class... ImageType>
      FORCE_INLINE Run<ImageType...> operator() (ImageType&... images) const { return { axes, std::tie (images...) }; }
  };

  struct LoopAlongDynamicAxesProgress : public LoopAlongDynamicAxes {
      const std::string& text;
      LoopAlongDynamicAxesProgress (const std::string& text, const std::vector<size_t>& axes) : 
        LoopAlongDynamicAxes ({ axes }), text (text) { }

      template <class... ImageType>
        struct Run : public LoopAlongDynamicAxes::Run<ImageType...> {
          MR::ProgressBar progress;
          FORCE_INLINE Run (const std::string& text, const std::vector<size_t> axes, const std::tuple<ImageType&...>& vox) : 
            LoopAlongDynamicAxes::Run<ImageType...> (axes, vox), progress (text, MR::voxel_count (std::get<0>(vox), axes)) { }
          FORCE_INLINE void operator++() { LoopAlongDynamicAxes::Run<ImageType...>::operator++(); ++progress; }
          FORCE_INLINE void operator++(int) { operator++(); }
        };

      template <class... ImageType>
        FORCE_INLINE Run<ImageType...> operator() (ImageType&... images) const { return { text, axes, std::tie (images...) }; }
    };



  FORCE_INLINE LoopAlongAxes Loop () { return { }; }
  FORCE_INLINE LoopAlongAxesProgress Loop (const std::string& progress_message) { return { progress_message }; }
  FORCE_INLINE LoopAlongSingleAxis Loop (size_t axis) { return { axis }; }
  FORCE_INLINE LoopAlongSingleAxisProgress Loop (const std::string& progress_message, size_t axis) { return { progress_message, axis }; }
  FORCE_INLINE LoopAlongAxisRange Loop (size_t axis_from, size_t axis_to) { return { axis_from, axis_to }; }
  FORCE_INLINE LoopAlongAxisRangeProgress Loop (const std::string& progress_message, size_t axis_from, size_t axis_to) { return { progress_message, axis_from, axis_to }; }
  FORCE_INLINE LoopAlongStaticAxes Loop (std::initializer_list<size_t> axes) { return { axes }; }
  FORCE_INLINE LoopAlongStaticAxesProgress Loop (const std::string& progress_message, std::initializer_list<size_t> axes) { return { progress_message, axes }; }
  FORCE_INLINE LoopAlongDynamicAxes Loop (std::vector<size_t> axes) { return { axes }; }
  FORCE_INLINE LoopAlongDynamicAxesProgress Loop (const std::string& progress_message, std::vector<size_t> axes) { return { progress_message, axes }; }


  //! @}
}

#endif


