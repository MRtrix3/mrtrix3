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

#include <variant>

#include "match_variant.h"
#include "stride.h"

namespace MR {

//! Request that an Image be backed by direct RAM access.
/*! Passed (wrapped in std::optional) to the Image factory functions
 * (Image::open(), Image::create(), Image::scratch(), Header::get_image())
 * to demand that the resulting Image use direct memory access for voxel
 * fetch / store, preloading from file into RAM if necessary.
 *
 * The default-constructed value requests direct IO with no constraints
 * on memory layout: a preload only occurs if the file's datatype, scaling
 * or segmentation forces it, otherwise the existing strides are kept.
 *
 * Construction from an integer requests direct IO with the specified \a axis
 * laid out contiguously in memory; a negative value (the SpatiallyContiguous
 * constant) requests that the spatial axes be contiguous.
 *
 * Construction from a Stride::List requests direct IO with the specified
 * explicit memory strides.
 *
 * The resulting Buffer is preloaded, if needed, during construction; once
 * the Image is observable (i.e. after the factory has returned), the
 * underlying RAM allocation is immutable until destruction. There is
 * therefore no need for runtime synchronisation between Image copies. */
class DirectIO {
public:
  DirectIO() = default;
  DirectIO(int axis) : request_(axis) {}
  DirectIO(Stride::List strides) : request_(std::move(strides)) {}

  //! Resolve the requested memory strides given a \a header.
  /*! Returns an empty Stride::List when no specific layout was requested
   * (i.e. when default-constructed). */
  template <class HeaderType> Stride::List resolve(const HeaderType &header) const {
    return MR::match_v(
        request_,
        [](std::monostate) { return Stride::List(); },
        [&](int axis) { return Stride::contiguous_along_axis(axis, header); },
        [](const Stride::List &list) { return list; });
  }

private:
  std::variant<std::monostate, int, Stride::List> request_;
};

} // namespace MR
