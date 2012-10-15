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

    /*! \addtogroup loop 
     * \{ */

    //! a class to loop over images in a multi-threaded fashion
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
              size_t num_axes_in_thread = 1) :
            loop (__get_axes_out_of_thread (axes_in_loop, num_axes_in_thread)),
            dummy (source),
            axes (__get_axes_in_thread (axes_in_loop, num_axes_in_thread)) {
              loop.start (dummy);
            }

        template <class InfoType>
          ThreadedLoop (
              const InfoType& source,
              size_t num_axes_in_thread = 1,
              size_t from_axis = 0,
              size_t to_axis = std::numeric_limits<size_t>::max()) :
            loop (__get_axes_out_of_thread (source, num_axes_in_thread, from_axis, to_axis)),
            dummy (source),
            axes (__get_axes_in_thread (source, num_axes_in_thread, from_axis, to_axis)) {
              loop.start (dummy);
            }

        template <class InfoType>
          ThreadedLoop (
              const std::string& message,
              const InfoType& source,
              const std::vector<size_t>& axes_out_of_thread,
              const std::vector<size_t>& axes_in_thread) :
            loop (axes_out_of_thread, message),
            dummy (source),
            axes (axes_in_thread) {
              loop.start (dummy);
            }

        template <class InfoType>
          ThreadedLoop (
              const std::string& message,
              const InfoType& source,
              const std::vector<size_t>& axes_in_loop,
              size_t num_axes_in_thread = 1) :
            loop (__get_axes_out_of_thread (axes_in_loop, num_axes_in_thread), message),
            dummy (source),
            axes (__get_axes_in_thread (axes_in_loop, num_axes_in_thread)) {
              loop.start (dummy);
            }

        template <class InfoType>
          ThreadedLoop (
              const std::string& message,
              const InfoType& source,
              size_t num_axes_in_thread = 1,
              size_t from_axis = 0,
              size_t to_axis = std::numeric_limits<size_t>::max()) :
            loop (__get_axes_out_of_thread (source, num_axes_in_thread, from_axis, to_axis), message),
            dummy (source),
            axes (__get_axes_in_thread (source, num_axes_in_thread, from_axis, to_axis)) {
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


        template <class Functor, class InputVoxelType> 
          static void apply (const std::string& progress_message, Functor functor, InputVoxelType& in);

        template <class OutputVoxelType, class Functor, class InputVoxelType> 
          static void apply (const std::string& progress_message, OutputVoxelType& out, Functor functor, InputVoxelType& in);

        template <class OutputVoxelType, class Functor, class InputVoxelType1, class InputVoxelType2> 
          static void apply (const std::string& progress_message, OutputVoxelType& out, Functor functor, InputVoxelType1& in1, InputVoxelType2& in2);

        template <class OutputVoxelType, class Functor, class InputVoxelType1, class InputVoxelType2, class InputVoxelType3> 
          static void apply (const std::string& progress_message, OutputVoxelType& out, Functor functor, InputVoxelType1& in1, InputVoxelType2& in2, InputVoxelType3& in3);

        template <class InputOutputVoxelType, class Functor> 
          static void apply_inplace (const std::string& progress_message, Functor functor, InputOutputVoxelType& in_out);

        template <class InputOutputVoxelType, class Functor, class InputVoxelType> 
          static void apply_inplace (const std::string& progress_message, Functor functor, InputOutputVoxelType& in_out, InputVoxelType& in);

        template <class InputOutputVoxelType, class Functor, class InputVoxelType1, class InputVoxelType2> 
          static void apply_inplace (const std::string& progress_message, Functor functor, InputOutputVoxelType& in_out, InputVoxelType1& in1, InputVoxelType2& in2);


      protected:
        LoopInOrder loop;
        Iterator dummy;
        const std::vector<size_t> axes;
        Thread::Mutex mutex;

        static std::vector<size_t> __get_axes_in_thread (
            const std::vector<size_t>& axes_in_loop,
            size_t num_axes_in_thread) {
          return std::vector<size_t> (axes_in_loop.begin(), axes_in_loop.begin()+num_axes_in_thread);
        }

        static std::vector<size_t> __get_axes_out_of_thread (
            const std::vector<size_t>& axes_in_loop,
            size_t num_axes_in_thread) {
          return std::vector<size_t> (axes_in_loop.begin()+num_axes_in_thread, axes_in_loop.end());
        }

        template <class InfoType>
          static std::vector<size_t> __get_axes_in_thread (
              const InfoType& source,
              size_t num_axes_in_thread,
              size_t from_axis,
              size_t to_axis) {
            return __get_axes_in_thread (Stride::order (source, from_axis, to_axis), num_axes_in_thread);
          }

        template <class InfoType>
          static std::vector<size_t> __get_axes_out_of_thread (
              const InfoType& source,
              size_t num_axes_in_thread,
              size_t from_axis,
              size_t to_axis) {
            return __get_axes_out_of_thread (Stride::order (source, from_axis, to_axis), num_axes_in_thread);
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
            LoopInOrder loop (shared.inner_axes());
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


    /*! \} */









    template <class Functor> void ThreadedLoop::run_outer (Functor functor, const std::string& thread_label)
    {
      ThreadedLoopKernelOuter<Functor> loop_thread (*this, functor);
      Thread::Array<ThreadedLoopKernelOuter<Functor> > thread_list (loop_thread);
      Thread::Exec threads (thread_list, thread_label);
    }




    template <class Functor> void ThreadedLoop::run (Functor functor, const std::string& thread_label)
    {
      ThreadedLoopKernelFull<Functor> loop_thread (*this, functor);
      Thread::Array<ThreadedLoopKernelFull<Functor> > thread_list (loop_thread);
      Thread::Exec threads (thread_list, thread_label);
    }





    //! \cond skip
    namespace {

      template <class Functor, class InfoType>
        inline void __apply (Functor functor, const InfoType& source_info, const std::string& progress_message)
        {
          if (progress_message.size()) ThreadedLoop (progress_message, source_info).run (functor, "apply thread");
          else ThreadedLoop (source_info).run (functor, "apply thread");
        }


      template <class Functor, class InputVoxelType>
        class __Functor1 {
          public:
            __Functor1 (Functor& functor, InputVoxelType& in) :
              func (functor), in (in) { } 
            void operator () (const Iterator& pos) {
              voxel_assign (in, pos);
              func (in.value());
            }
          protected:
            Functor func;
            InputVoxelType in;
        };
    }
    //! \endcond
    template <class Functor, class InputVoxelType> 
      inline void ThreadedLoop::apply (const std::string& progress_message, Functor functor, InputVoxelType& in)
      {
        __apply (__Functor1<Functor, InputVoxelType> (functor, in), in, progress_message);
      }



    //! \cond skip
    namespace {
      template <class Functor, class OutputVoxelType, class InputVoxelType>
        class __Functor2 {
          public:
            __Functor2 (Functor& functor, OutputVoxelType& out, InputVoxelType& in) :
              func (functor), out (out), in (in) { } 
            void operator () (const Iterator& pos) {
              voxel_assign (in, pos);
              voxel_assign (out, pos);
              out.value() = func (in.value());
            }
          protected:
            Functor func;
            OutputVoxelType out;
            InputVoxelType in;
        };
    }
    //! \endcond
    template <class OutputVoxelType, class Functor, class InputVoxelType> 
      inline void ThreadedLoop::apply (const std::string& progress_message, OutputVoxelType& out, Functor functor, InputVoxelType& in)
      {
        __apply (__Functor2<Functor, OutputVoxelType, InputVoxelType> (functor, out, in), in, progress_message);
      }



    //! \cond skip
    namespace {
      template <class Functor, class OutputVoxelType, class InputVoxelType1, class InputVoxelType2>
        class __Functor3 {
          public:
            __Functor3 (Functor& functor, OutputVoxelType& out, InputVoxelType1& in1, InputVoxelType2& in2) :
              func (functor), out (out), in1 (in1), in2 (in2) { } 
            void operator () (const Iterator& pos) {
              voxel_assign (in1, pos);
              voxel_assign (in2, pos);
              voxel_assign (out, pos);
              out.value() = func (in1.value(), in2.value());
            }
          protected:
            Functor func;
            OutputVoxelType out;
            InputVoxelType1 in1;
            InputVoxelType2 in2;
        };
    }
    //! \endcond
    template <class OutputVoxelType, class Functor, class InputVoxelType1, class InputVoxelType2> 
      void ThreadedLoop::apply (const std::string& progress_message, OutputVoxelType& out, Functor functor, InputVoxelType1& in1, InputVoxelType2& in2)
      {
        __apply (__Functor3<Functor, OutputVoxelType, InputVoxelType1, InputVoxelType2> (functor, out, in1, in2), in1, progress_message);
      }


    //! \cond skip
    namespace {
      template <class Functor, class OutputVoxelType, class InputVoxelType1, class InputVoxelType2, class InputVoxelType3>
        class __Functor4 {
          public:
            __Functor4 (Functor& functor, OutputVoxelType& out, InputVoxelType1& in1, InputVoxelType2& in2, InputVoxelType3& in3) : 
              func (functor), out (out), in1 (in1), in2 (in2), in3 (in3) { } 
            void operator () (const Iterator& pos) {
              voxel_assign (in1, pos);
              voxel_assign (in2, pos);
              voxel_assign (in3, pos);
              voxel_assign (out, pos);
              out.value() = func (in1.value(), in2.value());
            }
          protected:
            Functor func;
            OutputVoxelType out;
            InputVoxelType1 in1;
            InputVoxelType2 in2;
            InputVoxelType3 in3;
        };
    }
    //! \endcond
    template <class OutputVoxelType, class Functor, class InputVoxelType1, class InputVoxelType2, class InputVoxelType3> 
      void ThreadedLoop::apply (const std::string& progress_message, OutputVoxelType& out, Functor functor, InputVoxelType1& in1, InputVoxelType2& in2, InputVoxelType3& in3)
      {
        __apply (__Functor4<Functor, OutputVoxelType, InputVoxelType1, InputVoxelType2, InputVoxelType3> (functor, out, in1, in2, in3), in1, progress_message);
      }

    //! \cond skip
    namespace {
      template <class Functor, class InputOutputVoxelType>
        class __FunctorIP1 {
          public:
            __FunctorIP1 (Functor& functor, InputOutputVoxelType& in_out) : 
              func (functor), in_out (in_out) { } 
            void operator () (const Iterator& pos) {
              voxel_assign (in_out, pos);
              in_out.value() = func (in_out.value());
            }
          protected:
            Functor func;
            InputOutputVoxelType in_out;
        };
    }
    //! \endcond
    template <class InputOutputVoxelType, class Functor> 
      void ThreadedLoop::apply_inplace (const std::string& progress_message, Functor functor, InputOutputVoxelType& in_out)
      {
        __apply (__FunctorIP1<Functor, InputOutputVoxelType> (functor, in_out), in_out, progress_message);
      }


    //! \cond skip
    namespace {
      template <class Functor, class InputOutputVoxelType, class InputVoxelType>
        class __FunctorIP2 {
          public:
            __FunctorIP2 (Functor& functor, InputOutputVoxelType& in_out, InputVoxelType& in) : 
              func (functor), in_out (in_out), in (in) { } 
            void operator () (const Iterator& pos) {
              voxel_assign (in_out, pos);
              voxel_assign (in, pos);
              in_out.value() = func (in_out.value(), in.value());
            }
          protected:
            Functor func;
            InputOutputVoxelType in_out;
            InputVoxelType in;
        };
    }
    //! \endcond
    template <class InputOutputVoxelType, class Functor, class InputVoxelType> 
      void ThreadedLoop::apply_inplace (const std::string& progress_message, Functor functor, InputOutputVoxelType& in_out, InputVoxelType& in)
      {
        __apply (__FunctorIP2<Functor, InputOutputVoxelType, InputVoxelType> (functor, in_out, in), in_out, progress_message);
      }


    //! \cond skip
    namespace {
      template <class Functor, class InputOutputVoxelType, class InputVoxelType1, class InputVoxelType2>
        class __FunctorIP3 {
          public:
            __FunctorIP3 (Functor& functor, InputOutputVoxelType& in_out, InputVoxelType1& in1, InputVoxelType2& in2) : 
              func (functor), in_out (in_out), in1 (in1), in2 (in2) { } 
            void operator () (const Iterator& pos) {
              voxel_assign (in_out, pos);
              voxel_assign (in1, pos);
              voxel_assign (in2, pos);
              in_out.value() = func (in_out.value(), in1.value(), in2.value());
            }
          protected:
            Functor func;
            InputOutputVoxelType in_out;
            InputVoxelType1 in1;
            InputVoxelType2 in2;
        };
    }
    //! \endcond
    template <class InputOutputVoxelType, class Functor, class InputVoxelType1, class InputVoxelType2> 
      void ThreadedLoop::apply_inplace (const std::string& progress_message, Functor functor, InputOutputVoxelType& in_out, InputVoxelType1& in1, InputVoxelType2& in2)
      {
        __apply (__FunctorIP3<Functor, InputOutputVoxelType, InputVoxelType1, InputVoxelType2> (functor, in_out, in1, in2), in_out, progress_message);
      }

  }
}

#endif


