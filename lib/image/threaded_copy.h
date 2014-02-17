/*******************************************************************************
    Copyright (C) 2014 Brain Research Institute, Melbourne, Australia
    
    Permission is hereby granted under the Patent Licence Agreement between
    the BRI and Siemens AG from July 3rd, 2012, to Siemens AG obtaining a
    copy of this software and associated documentation files (the
    "Software"), to deal in the Software without restriction, including
    without limitation the rights to possess, use, develop, manufacture,
    import, offer for sale, market, sell, lease or otherwise distribute
    Products, and to permit persons to whom the Software is furnished to do
    so, subject to the following conditions:
    
    The above copyright notice and this permission notice shall be included
    in all copies or substantial portions of the Software.
    
    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
    OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
    MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
    IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
    CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
    TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
    SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

*******************************************************************************/


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
          .run_foreach (__copy_functor<OutputVoxelType, InputVoxelType>, Input(), source, Output(), destination);
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
          .run_foreach (__copy_functor<OutputVoxelType, InputVoxelType>, 
              source, Input(), 
              destination, Output());
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
          .run_foreach (__copy_functor<OutputVoxelType, InputVoxelType>, 
              source, Input(), 
              destination, Output());
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
          .run_foreach (__copy_functor<OutputVoxelType, InputVoxelType>, 
              source, Input(), 
              destination, Output());
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

