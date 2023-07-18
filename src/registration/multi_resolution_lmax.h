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

#ifndef __registration_multi_resolution_lmax_h__
#define __registration_multi_resolution_lmax_h__

#include "adapter/subset.h"
#include "adapter/extract.h"
#include "filter/smooth.h"
#include "registration/multi_contrast.h"

namespace MR
{
  namespace Registration
  {

    template <class ImageType>
    FORCE_INLINE ImageType multi_resolution_lmax (ImageType& input,
                                                  const default_type scale_factor,
                                                  const bool do_reorientation = false,
                                                  const uint32_t lmax = 0)
    {
      vector<uint32_t> from (input.ndim(), 0);
      vector<uint32_t> size (input.ndim());
      for (size_t dim = 0; dim < input.ndim(); ++dim)
        size[dim] = input.size(dim);
      if (do_reorientation)
        size[3] = Math::SH::NforL (lmax);
      Adapter::Subset<ImageType> subset (input, from, size);
      Filter::Smooth smooth_filter (subset);
      vector<default_type> stdev(3);
      for (size_t dim = 0; dim < 3; ++dim)
        stdev[dim] = input.spacing(dim) / (2.0 * scale_factor);

      smooth_filter.set_stdev (stdev);
      DEBUG ("creating scratch image for smoothing input image...");
      auto smoothed = ImageType::scratch (smooth_filter);
      threaded_copy (subset, smoothed);
      DEBUG ("smoothing input image based on scale factor...");
      smooth_filter (smoothed);
      return smoothed;
    }

    // crop and resize images as defined in contrast: contrast[tissue].start is relative to input,
    // contrast_updated[tissue].start is relative to cropped image. contrast and contrast_updated can be identical
    template <class ImageType>
    FORCE_INLINE ImageType multi_resolution_lmax (ImageType& input,
                                                  const default_type scale_factor,
                                                  const bool do_reorientation,
                                                  const vector<MultiContrastSetting>& contrast,
                                                  vector<MultiContrastSetting>* contrast_updated = nullptr)
    {
      vector<uint32_t> volume_indices;
      size_t start = 0;
      for (size_t ic = 0; ic < contrast.size(); ic++) {
        const auto & mc = contrast[ic];
        assert (mc.nvols > 0);
        for (size_t i = 0; i < mc.nvols; i++)
          volume_indices.push_back(mc.start + i);
        // adjust start index to be relative to subset
        if (contrast_updated)
          (*contrast_updated)[ic].start = start;
        start += mc.nvols;
      }
      Adapter::Extract1D<ImageType> subset (input, 3, volume_indices);

      Filter::Smooth smooth_filter (subset);
      vector<default_type> stdev(3);
      for (size_t dim = 0; dim < 3; ++dim)
        stdev[dim] = input.spacing(dim) / (2.0 * scale_factor);

      smooth_filter.set_stdev (stdev);
      DEBUG ("creating scratch image for smoothing input image...");
      auto smoothed = ImageType::scratch (smooth_filter);
      threaded_copy (subset, smoothed);
      DEBUG ("smoothing input image based on scale factor...");
      smooth_filter (smoothed);
      return smoothed;
    }
  }
}
#endif
