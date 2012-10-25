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

#include "image/threaded_loop.h"

namespace MR
{
  namespace Image
  {

    //! \cond skip
    namespace {

      template <class OutputVoxelType, class InputVoxelType>
        inline void __copy_functor (typename OutputVoxelType::value_type in, typename InputVoxelType::value_type& out) {
          out = in;
        }

    }
    
    //! \endcond




    template <class InputVoxelType, class OutputVoxelType>
      inline void threaded_copy (
          InputVoxelType& source, 
          OutputVoxelType& destination, 
          const std::vector<size_t>& axes,
          size_t num_axes_in_thread = 1) 
      {
        ThreadedLoop (source, axes, num_axes_in_thread)
          .foreach (__copy_functor<OutputVoxelType, InputVoxelType>, Input, source, Output, destination);
      }

    template <class InputVoxelType, class OutputVoxelType>
      inline void threaded_copy (
          InputVoxelType& source, 
          OutputVoxelType& destination, 
          size_t num_axes_in_thread = 1, 
          size_t from_axis = 0, 
          size_t to_axis = std::numeric_limits<size_t>::max())
      {
        ThreadedLoop (source, num_axes_in_thread, from_axis, to_axis)
          .foreach (__copy_functor<OutputVoxelType, InputVoxelType>, 
              source, Input, 
              destination, Output);
      }




    template <class InputVoxelType, class OutputVoxelType>
      inline void threaded_copy_with_progress_message (
          const std::string& message, 
          InputVoxelType& source, 
          OutputVoxelType& destination, 
          const std::vector<size_t>& axes,
          size_t num_axes_in_thread = 1)
      {
        ThreadedLoop (message, source, axes, num_axes_in_thread)
          .foreach (__copy_functor<OutputVoxelType, InputVoxelType>, 
              source, Input, 
              destination, Output);
      }

    template <class InputVoxelType, class OutputVoxelType>
      inline void threaded_copy_with_progress_message (
          const std::string& message, 
          InputVoxelType& source, 
          OutputVoxelType& destination, 
          size_t num_axes_in_thread = 1, 
          size_t from_axis = 0, 
          size_t to_axis = std::numeric_limits<size_t>::max())
      {
        ThreadedLoop (message, source, num_axes_in_thread, from_axis, to_axis)
          .foreach (__copy_functor<OutputVoxelType, InputVoxelType>, 
              source, Input, 
              destination, Output);
      }


    template <class InputVoxelType, class OutputVoxelType>
      inline void threaded_copy_with_progress (
          InputVoxelType& source,
          OutputVoxelType& destination, 
          const std::vector<size_t>& axes, 
          size_t num_axes_in_thread = 1)
      {
        threaded_copy_with_progress_message ("copying from \"" + shorten (source.name()) + "\" to \"" + shorten (destination.name()) + "\"...",
            source, destination, axes, num_axes_in_thread);
      }

    template <class InputVoxelType, class OutputVoxelType>
      inline void threaded_copy_with_progress (
          InputVoxelType& source, 
          OutputVoxelType& destination, 
          size_t num_axes_in_thread = 1,
          size_t from_axis = 0,
          size_t to_axis = std::numeric_limits<size_t>::max())
      {
        threaded_copy_with_progress_message ("copying from \"" + shorten (source.name()) + "\" to \"" + shorten (destination.name()) + "\"...",
            source, destination, num_axes_in_thread, from_axis, to_axis);
      }

  }
}

#endif

