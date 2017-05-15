/* Copyright (c) 2008-2017 the MRtrix3 contributors.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, you can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * MRtrix is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * For more details, see http://www.mrtrix.org/.
 */


#ifndef __algo_threaded_copy_h__
#define __algo_threaded_copy_h__

#include "algo/threaded_loop.h"

namespace MR
{

  //! \cond skip
  namespace {

    struct __copy_func { NOMEMALIGN
      template <class InputImageType, class OutputImageType>
        FORCE_INLINE void operator() (InputImageType& in, OutputImageType& out) const {
          out.value() = in.value();
        }
    };

  }

  //! \endcond




  template <class InputImageType, class OutputImageType>
    inline void threaded_copy (
        InputImageType& source, 
        OutputImageType& destination, 
        const vector<size_t>& axes,
        size_t num_axes_in_thread = 1) 
    {
      ThreadedLoop (source, axes, num_axes_in_thread)
        .run (__copy_func(), source, destination);
    }

  template <class InputImageType, class OutputImageType>
    inline void threaded_copy (
        InputImageType& source, 
        OutputImageType& destination, 
        size_t from_axis = 0, 
        size_t to_axis = std::numeric_limits<size_t>::max(),
        size_t num_axes_in_thread = 1)
    {
      ThreadedLoop (source, from_axis, to_axis, num_axes_in_thread)
        .run (__copy_func(), source, destination);
    }




  template <class InputImageType, class OutputImageType>
    inline void threaded_copy_with_progress_message (
        const std::string& message, 
        InputImageType& source, 
        OutputImageType& destination, 
        const vector<size_t>& axes,
        size_t num_axes_in_thread = 1)
    {
      ThreadedLoop (message, source, axes, num_axes_in_thread)
        .run (__copy_func(), source, destination);
    }

  template <class InputImageType, class OutputImageType>
    inline void threaded_copy_with_progress_message (
        const std::string& message, 
        InputImageType& source, 
        OutputImageType& destination, 
        size_t from_axis = 0, 
        size_t to_axis = std::numeric_limits<size_t>::max(), 
        size_t num_axes_in_thread = 1)
    {
      ThreadedLoop (message, source, from_axis, to_axis, num_axes_in_thread)
        .run (__copy_func(), source, destination);
    }


  template <class InputImageType, class OutputImageType>
    inline void threaded_copy_with_progress (
        InputImageType& source,
        OutputImageType& destination, 
        const vector<size_t>& axes, 
        size_t num_axes_in_thread = 1)
    {
      threaded_copy_with_progress_message ("copying from \"" + shorten (source.name()) + "\" to \"" + shorten (destination.name()) + "\"",
          source, destination, axes, num_axes_in_thread);
    }

  template <class InputImageType, class OutputImageType>
    inline void threaded_copy_with_progress (
        InputImageType& source, 
        OutputImageType& destination, 
        size_t from_axis = 0,
        size_t to_axis = std::numeric_limits<size_t>::max(),
        size_t num_axes_in_thread = 1)
    {
      threaded_copy_with_progress_message ("copying from \"" + shorten (source.name()) + "\" to \"" + shorten (destination.name()) + "\"",
          source, destination, from_axis, to_axis, num_axes_in_thread);
    }

}

#endif

