/* Copyright (c) 2008-2025 the MRtrix3 contributors.
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

#include "adapter/median.h"
#include "algo/threaded_copy.h"
#include "filter/base.h"
#include "image.h"

namespace MR::Filter {
/** \addtogroup Filters
@{ */

/*! Smooth images using median filtering.
 *
 * Typical usage:
 * \code
 * auto input = Image<float>::open (argument[0]);
 * Filter::Median median_filter (input);
 * auto output = Image<float>::create (argument[1], median_filter);
 * median_filter (input, output);
 *
 * \endcode
 */
class Median : public Base {

public:
  template <class HeaderType> Median(const HeaderType &in) : Base(in), extent(1, 3) { datatype() = DataType::Float32; }

  template <class HeaderType> Median(const HeaderType &in, std::string_view message) : Base(in, message), extent(1, 3) {
    datatype() = DataType::Float32;
  }

  template <class HeaderType>
  Median(const HeaderType &in, const std::vector<uint32_t> &extent) : Base(in), extent(extent) {
    datatype() = DataType::Float32;
  }

  template <class HeaderType>
  Median(const HeaderType &in, std::string_view message, const std::vector<uint32_t> &extent)
      : Base(in, message), extent(extent) {
    datatype() = DataType::Float32;
  }

  //! Set the extent of median filtering neighbourhood in voxels.
  //! This must be set as a single value for all three dimensions
  //! or three values, one for each dimension. Default 3x3x3.
  void set_extent(const std::vector<uint32_t> &ext) {
    for (size_t i = 0; i < ext.size(); ++i) {
      if (!(ext[i] & int(1)))
        throw Exception("expected odd number for extent");
    }
    extent = ext;
  }

  template <class InputImageType, class OutputImageType> void operator()(InputImageType &in, OutputImageType &out) {
    Adapter::Median<InputImageType> median(in, extent);
    if (message.empty())
      threaded_copy(median, out);
    else
      threaded_copy_with_progress_message(message, median, out);
  }

protected:
  std::vector<uint32_t> extent;
};
//! @}
} // namespace MR::Filter
