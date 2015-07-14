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

#ifndef __algo_copy_h__
#define __algo_copy_h__

#include "debug.h"
#include "algo/loop.h"

namespace MR
{

  template <class InputImageType, class OutputImageType>
    void copy (InputImageType&& source, OutputImageType&& destination, size_t from_axis = 0, size_t to_axis = std::numeric_limits<size_t>::max())
    {
      for (auto i = Loop (source, from_axis, to_axis) (source, destination); i; ++i) 
        destination.value() = source.value();
    }



  template <class InputImageType, class OutputImageType>
    void copy_with_progress (InputImageType&& source, OutputImageType&& destination, size_t from_axis = 0, size_t to_axis = std::numeric_limits<size_t>::max())
    {
      copy_with_progress_message ("copying from \"" + shorten (source.name()) + "\" to \"" + shorten (destination.name()) + "\"...",
          source, destination, from_axis, to_axis);
    }


  template <class InputImageType, class OutputImageType>
    void copy_with_progress_message (const std::string& message, InputImageType&& source, OutputImageType&& destination, size_t from_axis = 0, size_t to_axis = std::numeric_limits<size_t>::max())
    {
      for (auto i = Loop (message, source, from_axis, to_axis) (source, destination); i; ++i) 
        destination.value() = source.value();
    }


}

#endif
