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

#ifndef __image_threaded_copy_h__
#define __image_threaded_copy_h__

#include "debug.h"
#include "image/loop.h"
#include "image/iterator.h"
#include "thread/mutex.h"
#include "thread/exec.h"

namespace MR
{
  namespace Image
  {

    //! \cond skip
    namespace {

      template <class InputVoxelType, class OutputVoxelType> 
        class __ThreadedCopyInfo {
          public:
            __ThreadedCopyInfo (const std::string& message, const std::vector<size_t>& axes_out_of_thread, const std::vector<size_t>& axes_in_thread, InputVoxelType& source, OutputVoxelType& destination) :
              loop (axes_out_of_thread, message), 
              dummy (source),
              src (source),
              dest (destination),
              axes (axes_in_thread) {
                loop.start (dummy);
              }
            
            __ThreadedCopyInfo (const std::vector<size_t>& axes_out_of_thread, const std::vector<size_t>& axes_in_thread, InputVoxelType& source, OutputVoxelType& destination) :
              loop (axes_out_of_thread), 
              dummy (source),
              src (source),
              dest (destination),
              axes (axes_in_thread) {
                loop.start (dummy);
              }
            
            LoopInOrder loop;
            Iterator dummy;
            InputVoxelType& src;
            OutputVoxelType& dest;
            const std::vector<size_t>& axes;
            Thread::Mutex mutex;
          private:
            __ThreadedCopyInfo (const __ThreadedCopyInfo& a) { assert (0); }
        };



      template <class InputVoxelType, class OutputVoxelType> 
        class __ThreadedCopyAssign {
          public:
            __ThreadedCopyAssign (__ThreadedCopyInfo<InputVoxelType, OutputVoxelType>& common_info) :
              common (common_info) { }

            void execute () {
              InputVoxelType src (common.src);
              OutputVoxelType dest (common.dest);
              LoopInOrder loop (common.axes);

              while (get (src, dest)) {
                for (loop.start (src, dest); loop.ok(); loop.next (src, dest)) 
                  dest.value() = src.value();
              }
            }

          protected:
            __ThreadedCopyInfo<InputVoxelType, OutputVoxelType>& common;

            bool get (InputVoxelType& src, OutputVoxelType& dest) const {
              Thread::Mutex::Lock lock (common.mutex);
              if (common.loop.ok()) {
                common.loop.set_position (common.dummy, dest, src);
                common.loop.next (common.dummy);
                return true;
              }
              else 
                return false;
            }
        };

      template <class InputVoxelType> 
        inline void __get_axes (const InputVoxelType& source, size_t num_axes_in_thread, size_t from_axis, size_t to_axis, std::vector<size_t>& axes_out_of_thread, std::vector<size_t>& axes_in_thread) {
          axes_out_of_thread = Stride::order (source, from_axis, to_axis);
          assert (num_axes_in_thread < to_axis-from_axis);
          for (size_t n = 0; n < num_axes_in_thread; ++n) {
            axes_in_thread.push_back (axes_out_of_thread[0]);
            axes_out_of_thread.erase (axes_out_of_thread.begin());
          }
        }

    }
    
    //! \endcond




    template <class InputVoxelType, class OutputVoxelType>
      void threaded_copy (InputVoxelType& source, OutputVoxelType& destination, size_t num_axes_in_thread = 1, size_t from_axis = 0, size_t to_axis = std::numeric_limits<size_t>::max())
      {
        std::vector<size_t> axes_out_of_thread, axes_in_thread;
        __get_axes (source, num_axes_in_thread, from_axis, to_axis, axes_out_of_thread, axes_in_thread);
        __ThreadedCopyInfo<InputVoxelType, OutputVoxelType> common (axes_out_of_thread, axes_in_thread, source, destination);
        __ThreadedCopyAssign<InputVoxelType, OutputVoxelType> assign (common);
        Thread::Array<__ThreadedCopyAssign<InputVoxelType, OutputVoxelType> > thread_list (assign);
        Thread::Exec threads (thread_list, "copy thread");
      }



    template <class InputVoxelType, class OutputVoxelType>
      void threaded_copy_with_progress (InputVoxelType& source, OutputVoxelType& destination, size_t num_axes_in_thread = 1, size_t from_axis = 0, size_t to_axis = std::numeric_limits<size_t>::max())
      {
        threaded_copy_with_progress_message ("copying from \"" + shorten (source.name()) + "\" to \"" + shorten (destination.name()) + "\"...",
            source, destination, num_axes_in_thread, from_axis, to_axis);
      }



    template <class InputVoxelType, class OutputVoxelType>
      void threaded_copy_with_progress_message (
          const std::string& message, 
          InputVoxelType& source, 
          OutputVoxelType& destination, 
          size_t num_axes_in_thread = 1, 
          size_t from_axis = 0, 
          size_t to_axis = std::numeric_limits<size_t>::max())
      {
        std::vector<size_t> axes_out_of_thread, axes_in_thread;
        __get_axes (source, num_axes_in_thread, from_axis, to_axis, axes_out_of_thread, axes_in_thread);
        __ThreadedCopyInfo<InputVoxelType, OutputVoxelType> common (message, axes_out_of_thread, axes_in_thread, source, destination);
        __ThreadedCopyAssign<InputVoxelType, OutputVoxelType> assign (common);
        Thread::Array<__ThreadedCopyAssign<InputVoxelType, OutputVoxelType> > thread_list (assign);
        Thread::Exec threads (thread_list, "copy thread");
      }

  }
}

#endif

