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
  namespace Image
  {
    //! \cond skip
    namespace {

      class __ThreadShared {
        public:
          template <class InfoType>
            __ThreadShared (const std::vector<size_t>& axes_out_of_thread, const std::vector<size_t>& axes_in_thread, const InfoType& source) :
              loop (axes_out_of_thread),
              dummy (source),
              axes (axes_in_thread) {
                loop.start (dummy);
              }

          template <class InfoType>
            __ThreadShared (const std::string& message, const std::vector<size_t>& axes_out_of_thread, const std::vector<size_t>& axes_in_thread, const InfoType& source) :
              loop (axes_out_of_thread, message),
              dummy (source),
              axes (axes_in_thread) {
                loop.start (dummy);
              }

          LoopInOrder loop;
          Iterator dummy;
          const std::vector<size_t>& axes;
          Thread::Mutex mutex;
      };


      template <class Functor>
        class __ThreadedLoop {
          public:
            __ThreadedLoop (const Functor& functor, __ThreadShared& shared_info) :
              shared (shared_info),
              func (functor) { }

            void execute () {
              LoopInOrder loop (shared.axes);
              Iterator pos (shared.dummy);
              while (get (pos)) 
                for (loop.start (pos); loop.ok(); loop.next (pos)) 
                  func (pos);
            }


        protected:
            __ThreadShared& shared;
            Functor func;

            bool get (Iterator& pos) const {
              Thread::Mutex::Lock lock (shared.mutex);
              if (shared.loop.ok()) {
                shared.loop.set_position (shared.dummy, pos);
                shared.loop.next (shared.dummy);
                return true;
              }
              else return false;
            }
      };


    }

    //! \endcond




    template <class Functor, class InfoType>
      void threaded_loop (const Functor& functor, const InfoType& source, const std::vector<size_t>& axes, size_t num_axes_in_thread = 1) 
      {
        const std::vector<size_t> axes_out_of_thread (axes.begin()+num_axes_in_thread, axes.end());
        const std::vector<size_t> axes_in_thread (axes.begin(), axes.begin()+num_axes_in_thread);
        __ThreadShared shared (axes_out_of_thread, axes_in_thread, source);
        __ThreadedLoop<Functor> loop_thread (functor, shared);
        Thread::Array<__ThreadedLoop<Functor> > thread_list (loop_thread);
        Thread::Exec threads (thread_list, "loop thread");
      }

    template <class Functor, class InfoType>
      void threaded_loop (const Functor& functor, const InfoType& source, size_t num_axes_in_thread = 1, size_t from_axis = 0, size_t to_axis = std::numeric_limits<size_t>::max())
      {
        const std::vector<size_t> axes = Stride::order (source, from_axis, to_axis);
        threaded_loop (functor, source, axes, num_axes_in_thread);
      }




    template <class Functor, class InfoType>
      void threaded_loop_with_progress_message (
          const std::string& message, 
          const Functor& functor, 
          const InfoType& source, 
          const std::vector<size_t>& axes,
          size_t num_axes_in_thread = 1)
      {
        const std::vector<size_t> axes_out_of_thread (axes.begin()+num_axes_in_thread, axes.end());
        const std::vector<size_t> axes_in_thread (axes.begin(), axes.begin()+num_axes_in_thread);
        __ThreadShared shared (message, axes_out_of_thread, axes_in_thread, source);
        __ThreadedLoop<Functor> loop_thread (functor, shared);
        Thread::Array<__ThreadedLoop<Functor> > thread_list (loop_thread);
        Thread::Exec threads (thread_list, "loop thread");
      }

    template <class Functor, class InfoType>
      void threaded_loop_with_progress_message (
          const std::string& message, 
          const Functor& functor,
          const InfoType& source, 
          size_t num_axes_in_thread = 1, 
          size_t from_axis = 0, 
          size_t to_axis = std::numeric_limits<size_t>::max())
      {
        const std::vector<size_t> axes = Stride::order (source, from_axis, to_axis);
        threaded_loop_with_progress_message (message, functor, source, axes, num_axes_in_thread);
      }


  }
}

#endif


