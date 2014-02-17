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
