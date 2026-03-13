/* Copyright (c) 2008-2026 the MRtrix3 contributors.
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

#pragma once

#include "algo/loop.h"
#include "types.h"

namespace MR {

template <class InputImageType, class OutputImageType>
void copy(InputImageType &&source,
          OutputImageType &&destination,
          const ArrayIndex from_axis = 0,
          const ArrayIndex to_axis = -1) {
  for (auto i = Loop(source, from_axis, to_axis)(source, destination); i; ++i)
    destination.value() = source.value();
}

template <class InputImageType, class OutputImageType>
void copy_with_progress(InputImageType &&source,
                        OutputImageType &&destination,
                        const ArrayIndex from_axis = 0,
                        const ArrayIndex to_axis = -1) {
  copy_with_progress_message("copying from \"" + shorten(source.name()) + "\" to \"" + shorten(destination.name()) +
                                 "\"...",
                             source,
                             destination,
                             from_axis,
                             to_axis);
}

template <class InputImageType, class OutputImageType>
void copy_with_progress_message(std::string_view message,
                                InputImageType &&source,
                                OutputImageType &&destination,
                                const ArrayIndex from_axis = 0,
                                const ArrayIndex to_axis = -1) {
  for (auto i = Loop(message, source, from_axis, to_axis)(source, destination); i; ++i)
    destination.value() = source.value();
}

} // namespace MR
