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


#ifndef __image_adapter_gaussian1D_h__
#define __image_adapter_gaussian1D_h__

#include "adapter/base.h"

namespace MR
{
  namespace Adapter
  {

    template <class ImageType>
      class Gaussian1D : 
        public Base<Gaussian1D<ImageType>,ImageType> 
    { MEMALIGN (Gaussian1D<ImageType>) 
      public:

        using base_type = Base<Gaussian1D<ImageType>, ImageType>;
        using value_type = typename ImageType::value_type;

        using base_type::name;
        using base_type::size;
        using base_type::spacing;
        using base_type::index;


        Gaussian1D (const ImageType& parent,
                    default_type stdev_in = 1.0,
                    size_t axis_in = 0,
                    size_t extent = 0,
                    bool zero_boundary = false) :
          base_type (parent),
          stdev (stdev_in),
          axis (axis_in),
          zero_boundary (zero_boundary) {
          if (!extent)
            radius = ceil(2 * stdev / spacing(axis));
          else if (extent == 1)
            radius = 0;
          else
            radius = (extent - 1) / 2;
          compute_kernel();
        }


        value_type value ()
        {
          if (!kernel.size())
            return base_type::value();

          const ssize_t pos = index (axis);

          if (zero_boundary)
            if (pos == 0 || pos == size(axis) - 1)
              return 0.0;

          const ssize_t from = (pos < radius) ? 0 : pos - radius;
          const ssize_t to = (pos + radius) >= size(axis) ? size(axis) - 1 : pos + radius;

          value_type result = 0.0;
          value_type av_weights = 0.0;
          ssize_t c = (pos < radius) ? radius - pos : 0;
          for (ssize_t k = from; k <= to; ++k, ++c) {
            index (axis) = k;
            value_type neighbour_value = base_type::value();
            if (std::isfinite (neighbour_value)) {
              av_weights += kernel[c];
              result += value_type (base_type::value()) * kernel[c];
            }
          }
          result /= av_weights;

          index (axis) = pos;
          return result;
        }

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
        vector<default_type> kernel;
        const bool zero_boundary;
      };
  }
}


#endif

