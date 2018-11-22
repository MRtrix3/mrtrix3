/* Copyright (c) 2008-2019 the MRtrix3 contributors.
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
#include "filter/smooth.h"

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
  }
}
#endif
