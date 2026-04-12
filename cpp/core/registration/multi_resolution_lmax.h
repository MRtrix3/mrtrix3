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

#include "adapter/extract.h"
#include "adapter/subset.h"
#include "filter/smooth.h"
#include "registration/multi_contrast.h"

namespace MR::Registration {

namespace {
template <class ImageType>
Image<typename ImageType::value_type> smooth(ImageType &input, const default_type scale_factor) {
  Filter::Smooth smooth_filter(input);
  std::vector<default_type> stdev(3);
  for (ArrayIndex dim = 0; dim < 3; ++dim)
    stdev[dim] = input.spacing(dim) / (2.0 * scale_factor);
  smooth_filter.set_stdev(stdev);
  DEBUG("creating scratch image for smoothing input image...");
  auto smoothed = Image<typename ImageType::value_type>::scratch(smooth_filter);
  threaded_copy(input, smoothed);
  DEBUG("smoothing input image based on scale factor...");
  smooth_filter(smoothed);
  return smoothed;
}
} // namespace

template <class ImageType>
ImageType multi_resolution_lmax(ImageType &input,
                                const default_type scale_factor,
                                const bool do_reorientation = false,
                                const uint32_t lmax = 0) {
  std::vector<VoxelIndex> from(input.ndim(), 0);
  std::vector<VoxelIndex> size(input.ndim());
  for (ArrayIndex dim = 0; dim < input.ndim(); ++dim)
    size[dim] = input.size(dim);
  if (do_reorientation)
    size[3] = Math::SH::NforL(lmax);
  Adapter::Subset<ImageType> subset(input, from, size);
  return smooth(subset, scale_factor);
}

// crop and resize images as defined in contrast: contrast[tissue].start is relative to input,
// contrast_updated[tissue].start is relative to cropped image. contrast and contrast_updated can be identical
template <class ImageType>
ImageType multi_resolution_lmax(ImageType &input,
                                const default_type scale_factor,
                                const bool do_reorientation,
                                const std::vector<MultiContrastSetting> &contrast,
                                std::vector<MultiContrastSetting> *contrast_updated = nullptr) {

  if (input.ndim() == 3)
    return smooth(input, scale_factor);

  std::vector<VoxelIndex> volume_indices;
  VoxelIndex start = 0;
  for (size_t ic = 0; ic < contrast.size(); ic++) {
    const auto &mc = contrast[ic];
    assert(mc.nvols > 0);
    for (VoxelIndex i = 0; i < mc.nvols; i++)
      volume_indices.push_back(mc.start + i);
    // adjust start index to be relative to subset
    if (contrast_updated)
      (*contrast_updated)[ic].start = start;
    start += mc.nvols;
  }
  Adapter::Extract1D<ImageType> subset(input, 3, volume_indices);
  return smooth(subset, scale_factor);
}

} // namespace MR::Registration
