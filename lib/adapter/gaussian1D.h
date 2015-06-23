/*
    Copyright 2008 Brain Research Institute, Melbourne, Australia

    Written by David Raffelt, 17/02/12.

    This file is part of MRtrix.

    MRtrix is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    MRtrix is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with MRtrix.  If not, see <http://www.gnu.org/licenses/>.

 */

#ifndef __image_adapter_gaussian1D_h__
#define __image_adapter_gaussian1D_h__

#include "adapter/base.h"

namespace MR
{
  namespace Adapter
  {

    template <class ImageType>
      class Gaussian1D : public Base<ImageType> {
      public:
        Gaussian1D (const ImageType& parent,
                    default_type stdev_in = 1.0,
                    size_t axis_in = 0,
                    size_t extent = 0) :
          Base<ImageType> (parent),
          stdev (stdev_in),
          axis (axis_in) {
          if (!extent)
            radius = ceil(2.5 * stdev / voxsize(axis));
          else if (extent == 1)
            radius = 0;
          else
            radius = (extent - 1) / 2;
          compute_kernel();
        }

        typedef typename ImageType::value_type value_type;

        value_type& value ()
        {
          if (!kernel.size()) {
            result = Base<ImageType>::value();
            return result;
          }

          const ssize_t pos = index (axis);
          const int from = pos < radius ? 0 : pos - radius;
          const int to = pos >= (size(axis) - radius) ? size(axis) - 1 : pos + radius;

          result = 0.0;

          if (pos < radius) {
            size_t c = radius - pos;
            value_type av_weights = 0.0;
            for (ssize_t k = from; k <= to; ++k) {
              av_weights += kernel[c];
              index (axis) = k;
              result += value_type (Base<ImageType>::value()) * kernel[c++];
            }
            result /= av_weights;
          } else if ((to - pos) < radius){
            size_t c = 0;
            value_type av_weights = 0.0;
            for (ssize_t k = from; k <= to; ++k) {
              av_weights += kernel[c];
              index (axis) = k;
              result += value_type (Base<ImageType>::value()) * kernel[c++];
            }
            result /= av_weights;
          } else {
            size_t c = 0;
            for (ssize_t k = from; k <= to; ++k) {
              index (axis) = k;
              result += value_type (Base<ImageType>::value()) * kernel[c++];
            }
          }

          index (axis) = pos;
          return result;
        }

        using Base<ImageType>::name;
        using Base<ImageType>::size;
        using Base<ImageType>::voxsize;
        using Base<ImageType>::index;

      protected:

        void compute_kernel()
        {
          if ((radius < 1) || stdev <= 0.0)
            return;
          kernel.resize(2 * radius + 1);
          default_type norm_factor = 0.0;
          for (size_t c = 0; c < kernel.size(); ++c) {
            kernel[c] = exp(-((c-radius) * (c-radius) * voxsize(axis) * voxsize(axis))  / (2 * stdev * stdev));
            norm_factor += kernel[c];
          }
          for (size_t c = 0; c < kernel.size(); c++) {
            kernel[c] /= norm_factor;
          }
        }

        default_type stdev;
        ssize_t radius;
        size_t axis;
        std::vector<default_type> kernel;
        value_type result;
      };
  }
}


#endif

