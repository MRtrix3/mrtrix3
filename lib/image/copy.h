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


#ifndef __image_copy_h__
#define __image_copy_h__

#include "debug.h"
#include "image/loop.h"

namespace MR
{
  namespace Image
  {

    template <class InputVoxelType, class OutputVoxelType>
    void copy (InputVoxelType& source, OutputVoxelType& destination, size_t from_axis = 0, size_t to_axis = std::numeric_limits<size_t>::max())
    {
      LoopInOrder loop (source, from_axis, to_axis);
      for (loop.start (source, destination); loop.ok(); loop.next (source, destination))
        destination.value() = source.value();
    }



    template <class InputVoxelType, class OutputVoxelType>
    void copy_with_progress (InputVoxelType& source, OutputVoxelType& destination, size_t from_axis = 0, size_t to_axis = std::numeric_limits<size_t>::max())
    {
      copy_with_progress_message ("copying from \"" + shorten (source.name()) + "\" to \"" + shorten (destination.name()) + "\"...",
         source, destination, from_axis, to_axis);
    }


    template <class InputVoxelType, class OutputVoxelType>
    void copy_with_progress_message (const std::string& message, InputVoxelType& source, OutputVoxelType& destination, size_t from_axis = 0, size_t to_axis = std::numeric_limits<size_t>::max())
    {
      LoopInOrder loop (source, message, from_axis, to_axis);
      for (loop.start (source, destination); loop.ok(); loop.next (source, destination))
        destination.value() = source.value();
    }

  }
}

#endif
