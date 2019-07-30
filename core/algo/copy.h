/*
 * Copyright (c) 2008-2018 the MRtrix3 contributors.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, you can obtain one at http://mozilla.org/MPL/2.0/
 *
 * MRtrix3 is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * For more details, see http://www.mrtrix.org/
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
