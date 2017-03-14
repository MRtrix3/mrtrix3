/* Copyright (c) 2008-2017 the MRtrix3 contributors
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
                                                  const int lmax = 0)
    {
      vector<int> from (input.ndim(), 0);
      vector<int> size (input.ndim());
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

    template <class ImageType>
    FORCE_INLINE ImageType multi_resolution_lmax (ImageType& input,
                                                  const default_type scale_factor,
                                                  const bool do_reorientation,
                                                  const vector<MultiContrastSetting>& contrast)
    {
      vector<int> volume_indices;
      for (auto const& mc : contrast) {
        volume_indices.push_back((int) mc.start);
        for (size_t i = 1; i < mc.nvols; i++)
          volume_indices.push_back((int) (mc.start + i));
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
