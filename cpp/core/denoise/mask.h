/* Copyright (c) 2008-2024 the MRtrix3 contributors.
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

#include <limits>

#include "algo/loop.h"
#include "datatype.h"
#include "exception.h"
#include "header.h"
#include "image.h"

namespace MR::Denoise {

// Need to sweep through the input data,
//   identify voxels that cannot be utilised in PCA,
//   and generate a mask that will preclude them from contributing
// This can only be done after an Image<> instance has been created,
//   which is typically templated based on data / user input

// TODO These functions should also take an optional VST image as input,
//   and exclude from the mask those voxels where a valid noise level reading can't be obtained
// Note however that this may change between iterations,
//   which conflicts with how this is currently managed,
//   where the mask is computed only once before the first iteration

template <typename T> typename std::enable_if<is_complex<T>::value, Image<bool>>::type generate_mask(Image<T> &image) {
  Header H(image);
  H.ndim() = 3;
  H.datatype() = DataType::Bit;
  Image<bool> mask = Image<bool>::scratch(H, "Scratch mask of voxels with valid data for denoising");
  size_t excluded_count(0);
  for (auto l_voxel = Loop(mask)(image, mask); l_voxel; ++l_voxel) {
    T min_value(std::numeric_limits<typename T::value_type>::infinity(),
                std::numeric_limits<typename T::value_type>::infinity());
    T max_value(-std::numeric_limits<typename T::value_type>::infinity(),
                -std::numeric_limits<typename T::value_type>::infinity());
    bool all_finite = true;
    for (auto l_inner = Loop(image, 3)(image); l_inner; ++l_inner) {
      if (!std::isfinite(T(image.value()).real()) || !std::isfinite(T(image.value()).imag())) {
        all_finite = false;
      } else {
        min_value = {std::min(min_value.real(), T(image.value()).real()),
                     std::min(min_value.imag(), T(image.value()).imag())};
        max_value = {std::max(max_value.real(), T(image.value()).real()),
                     std::max(max_value.imag(), T(image.value()).imag())};
      }
    }
    if (all_finite && min_value != max_value)
      mask.value() = true;
    else
      ++excluded_count;
  }
  if (excluded_count > 0) {
    WARN(str(excluded_count) + " voxels were found with invalid data;"
                               " these will be excluded from processing");
  }
  return mask;
}

template <typename T> typename std::enable_if<!is_complex<T>::value, Image<bool>>::type generate_mask(Image<T> &image) {
  Header H(image);
  H.ndim() = 3;
  H.datatype() = DataType::Bit;
  Image<bool> mask = Image<bool>::scratch(H, "Scratch mask of voxels with valid data for denoising");
  size_t excluded_count(0);
  for (auto l_voxel = Loop(mask)(image, mask); l_voxel; ++l_voxel) {
    T min_value(std::numeric_limits<T>::infinity());
    T max_value(-std::numeric_limits<T>::infinity());
    bool all_finite = true;
    for (auto l_inner = Loop(image, 3)(image); l_inner; ++l_inner) {
      if (!std::isfinite(image.value())) {
        all_finite = false;
      } else {
        min_value = std::min(min_value, T(image.value()));
        max_value = std::max(max_value, T(image.value()));
      }
    }
    if (all_finite && min_value != max_value)
      mask.value() = true;
    else
      ++excluded_count;
  }
  if (excluded_count > 0) {
    WARN("A total of " + str(excluded_count) +
         " voxels were found with invalid data;"
         " these will be excluded from processing");
  }
  return mask;
}

} // namespace MR::Denoise
