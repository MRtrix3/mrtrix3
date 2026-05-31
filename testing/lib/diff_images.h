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

#include <cstdint>

#include "datatype.h"
#include "header.h"
#include "progressbar.h"

#include "image.h"
#include "image_diff.h"
#include "image_helpers.h"

#include "adapter/replicate.h"

namespace MR::Testing {

// clang-format off
const App::OptionGroup Diff_Image_Options =
    App::OptionGroup ("Testing image options")
    + App::Option ("abs", "specify an absolute tolerance")
      + App::Argument ("tolerance").type_float(0.0)
    + App::Option ("frac", "specify a fractional tolerance")
      + App::Argument ("tolerance").type_float(0.0)
    + App::Option ("image", "specify an image containing the tolerances")
      + App::Argument ("path").type_image_in()
    + App::Option ("voxel", "specify a fractional tolerance relative to the maximum value in the voxel")
      + App::Argument ("tolerance").type_float(0.0);
// clang-format on

template <class ImageType1, class ImageType2> void diff_images(ImageType1 &in1, ImageType2 &in2) {

  auto abs_opt = App::get_options("abs");
  auto frac_opt = App::get_options("frac");
  auto image_opt = App::get_options("image");
  auto voxel_opt = App::get_options("voxel");

  if (!abs_opt.empty()) {
    check_images_abs(in1, in2, abs_opt[0][0]);
  } else if (!frac_opt.empty()) {
    check_images_frac(in1, in2, frac_opt[0][0]);
  } else if (!image_opt.empty()) {
    auto tolerance = Image<default_type>::open(image_opt[0][0]);
    Adapter::Replicate<decltype(tolerance)> replicate(tolerance, in1);
    check_images_tolimage(in1, in2, replicate);
  } else if (!voxel_opt.empty()) {
    check_images_voxel(in1, in2, voxel_opt[0][0]);
  } else {
    check_images_abs(in1, in2, 0.0);
  }
}

//! whether the comparison between two images of given data types must be performed bitwise
/*! Two images stored with the Bit data type are compared exactly;
 *  no comparison tolerance is applicable in this scenario.
 */
inline bool is_bitwise_comparison(const DataType dt1, const DataType dt2) {
  return dt1.is(DataType::Bit) && dt2.is(DataType::Bit);
}

//! determine the comparison data type appropriate for two image data types
/*! The comparison is performed at the highest width / precision of the two inputs;
 *  values are only promoted to complex floating-point if at least one input is truly complex,
 *  and integer inputs are compared using integer arithmetic.
 */
inline DataType comparison_datatype(const DataType dt1, const DataType dt2) {
  if (dt1.is_complex() || dt2.is_complex())
    return DataType::CFloat64;
  if (dt1.is_floating_point() || dt2.is_floating_point())
    return DataType::Float64;
  if (dt1.is_signed() || dt2.is_signed())
    return DataType::Int64;
  return DataType::UInt64;
}

//! read two image headers, determine the appropriate comparison data type, and check the images for differences
inline void diff_images(Header &header1, Header &header2) {

  if (is_bitwise_comparison(header1.datatype(), header2.datatype())) {
    for (const std::string &name : {"abs", "frac", "image", "voxel"}) {
      if (!App::get_options(name).empty())
        throw Exception("Comparison tolerance option -" + name +
                        " is not applicable to bitwise comparison of Bit data type images");
    }
    auto in1 = header1.get_image<bool>();
    auto in2 = header2.get_image<bool>();
    check_images_bitwise(in1, in2);
    return;
  }

  switch (comparison_datatype(header1.datatype(), header2.datatype())()) {
  case DataType::CFloat64: {
    auto in1 = header1.get_image<cdouble>();
    auto in2 = header2.get_image<cdouble>();
    diff_images(in1, in2);
  } break;
  case DataType::Float64: {
    auto in1 = header1.get_image<double>();
    auto in2 = header2.get_image<double>();
    diff_images(in1, in2);
  } break;
  case DataType::Int64: {
    auto in1 = header1.get_image<int64_t>();
    auto in2 = header2.get_image<int64_t>();
    diff_images(in1, in2);
  } break;
  default: {
    auto in1 = header1.get_image<uint64_t>();
    auto in2 = header2.get_image<uint64_t>();
    diff_images(in1, in2);
  } break;
  }
}
} // namespace MR::Testing
