/* Copyright (c) 2008-2023 the MRtrix3 contributors.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Covered Software is provided under this License on an "as is"
 * basis, without warranty of any kind, either expressed, implied, or
 * statutory, including, without limitation, warranties that the
 * Covered Software is free of defects, merchantable, fit for a
 * particular purpose or non-infringing.
 * See the Mozilla Public License v. 2.0 for more details.
 *
 * For more details, see http://www.mrtrix.org/.
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
