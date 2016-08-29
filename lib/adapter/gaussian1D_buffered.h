/*
 * Copyright (c) 2008-2016 the MRtrix3 contributors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/
 *
 * MRtrix is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * For more details, see www.mrtrix.org
 *
 */

#ifndef __image_adapter_gaussian1D_buffered_h__
#define __image_adapter_gaussian1D_buffered_h__

#include "adapter/base.h"

namespace MR
{
  namespace Adapter
  {

    template <class ImageType>
      class Gaussian1DBuffered : public Base<ImageType> {
      public:
        Gaussian1DBuffered (const ImageType& parent,
                    default_type stdev_in = 1.0,
                    size_t axis_in = 0,
                    size_t extent = 0,
                    bool zero_boundary = false) :
          Base<ImageType> (parent),
          stdev (stdev_in),
          axis (axis_in),
          zero_boundary (zero_boundary),
          pos_prev (std::numeric_limits<ssize_t>::max()) {
          if (!extent)
            radius = ceil(2 * stdev / spacing(axis));
          else if (extent == 1)
            radius = 0;
          else
            radius = (extent - 1) / 2;
          compute_kernel();
          buffer_size = size(axis);
          buffer.resize(buffer_size);
        }

        typedef typename ImageType::value_type value_type;

        value_type value ()
        {
          if (!kernel.size())
            return Base<ImageType>::value();

          const ssize_t pos = index (axis);
          assert (pos_prev != pos && "Loop axis has to be equal to smoothing axis");

          // fill buffer for current image line if necessary
          if (pos == 0) {
            assert (pos == 0);
            for (ssize_t k = 0; k < buffer_size; ++k) {
              index(axis) = k;
              buffer(k) = Base<ImageType>::value();
            }
            index (axis) = pos;
          } else {
            assert (pos_prev + 1 == pos && "Loop not contiguous along smoothing axis");
          }

          if (zero_boundary)
            if (pos == 0 || pos == size(axis) - 1)
              return 0.0;

          const ssize_t from = (pos < radius) ? 0 : pos - radius;
          const ssize_t to = (pos + radius) >= size(axis) ? size(axis) - 1 : pos + radius;

          ssize_t c = (pos < radius) ? radius - pos : 0;
          ssize_t kernel_size = to - from + 1;

          value_type result = kernel.segment(c, kernel_size).dot(buffer.segment(from, kernel_size));

          if (!std::isfinite(result)) {
            result = 0.0;
            value_type av_weights = 0.0;
            for (ssize_t k = from; k <= to; ++k, ++c) {
              value_type neighbour_value = buffer(k);
              if (std::isfinite (neighbour_value)) {
                av_weights += kernel[c];
                result += neighbour_value * kernel[c];
              }
            }
            result /= av_weights;
          } else if (kernel_size != kernel.size())
              result /= kernel.segment(c, kernel_size).sum();

          pos_prev = pos;

          return result;
        }

        using Base<ImageType>::name;
        using Base<ImageType>::size;
        using Base<ImageType>::spacing;
        using Base<ImageType>::index;

      protected:

        void compute_kernel()
        {
          if ((radius < 1) || stdev <= 0.0)
            return;
          kernel.resize (2 * radius + 1);
          default_type norm_factor = 0.0;
          for (size_t c = 0; c < kernel.size(); ++c) {
            kernel[c] = exp(-((c-radius) * (c-radius) * spacing(axis) * spacing(axis))  / (2 * stdev * stdev));
            norm_factor += kernel[c];
          }
          for (size_t c = 0; c < kernel.size(); c++) {
            kernel[c] /= norm_factor;
          }
        }

        default_type stdev;
        ssize_t radius;
        size_t axis;
        Eigen::VectorXd kernel;
        const bool zero_boundary;
        ssize_t pos_prev;
        ssize_t buffer_size;
        Eigen::VectorXd buffer;
      };
  }
}


#endif

