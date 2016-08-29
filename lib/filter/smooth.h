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

#ifndef __image_filter_gaussian_h__
#define __image_filter_gaussian_h__

#include "memory.h"
#include "image.h"
#include "algo/copy.h"
#include "algo/threaded_copy.h"
#include "adapter/gaussian1D.h"
#include "adapter/gaussian1D_buffered.h"
#include "filter/base.h"

namespace MR
{
  namespace Filter
  {
    /** \addtogroup Filters
    @{ */

    /*! Smooth images using a Gaussian kernel.
     *
     * Typical usage:
     * \code
     * auto input = Image<float>::open (argument[0]);
     * Filter::Smooth smooth_filter (input);
     * smooth_filter.set_stdev (2.0);
     * auto output = Image::create<float> (argument[1], smooth_filter);
     * smooth_filter (input, output);
     *
     * \endcode
     */
    class Smooth : public Base
    {

      public:
        template <class HeaderType>
        Smooth (const HeaderType& in) :
            Base (in),
            extent (3, 0),
            stdev (3, 0.0),
            stride_order (Stride::order (in)),
            zero_boundary (false)
        {
          for (int i = 0; i < 3; i++)
            stdev[i] = in.spacing(i);
          datatype() = DataType::Float32;
        }

        template <class HeaderType>
        Smooth (const HeaderType& in, const std::vector<default_type>& stdev_in):
            Base (in),
            extent (3, 0),
            stdev (3, 0.0),
            stride_order (Stride::order (in))
        {
          set_stdev (stdev_in);
          datatype() = DataType::Float32;
        }

        //! Set the extent of smoothing kernel in voxels.
        //! This can be set as a single value to be used for the first 3 dimensions
        //! or separate values, one for each dimension. (Default: 4 standard deviations)
        void set_extent (const std::vector<int>& new_extent)
        {
          if (new_extent.size() != 1 && new_extent.size() != 3)
            throw Exception ("Please supply a single kernel extent value, or three values (one for each spatial dimension)");
          for (size_t i = 0; i < new_extent.size(); ++i) {
            if (!(new_extent[i] & int (1)))
              throw Exception ("expected odd number for extent");
            if (new_extent[i] < 0)
              throw Exception ("the kernel extent must be positive");
          }
          if (new_extent.size() == 1)
            for (unsigned int i = 0; i < 3; i++)
              extent[i] = new_extent[0];
          else
            extent = new_extent;
        }

        void set_stdev (default_type stdev_in) {
          set_stdev (std::vector<default_type> (3, stdev_in));
        }

        //! ensure the image boundary remains zero. Used to constrain displacement fields during image registration
        void set_zero_boundary (bool do_zero_boundary) {
          zero_boundary = do_zero_boundary;
        }

        //! Set the standard deviation of the Gaussian defined in mm.
        //! This must be set as a single value to be used for the first 3 dimensions
        //! or separate values, one for each dimension. (Default: 1 voxel)
        void set_stdev (const std::vector<default_type>& std_dev)
        {
          for (size_t i = 0; i < std_dev.size(); ++i)
            if (stdev[i] < 0.0)
              throw Exception ("the Gaussian stdev values cannot be negative");
          if (std_dev.size() == 1) {
            for (unsigned int i = 0; i < 3; i++)
              stdev[i] = std_dev[0];
          } else {
            if (std_dev.size() != 3)
              throw Exception ("Please supply a single standard deviation value, or three values (one for each spatial dimension)");
            for (unsigned int i = 0; i < 3; i++)
              stdev[i] = std_dev[i];
          }
        }

        //! Smooth the input image. Both input and output images can be the same image
        template <class InputImageType, class OutputImageType, typename ValueType = float>
        void operator() (InputImageType& input, OutputImageType& output)
        {
          std::shared_ptr <Image<ValueType> > in (std::make_shared<Image<ValueType> > (Image<ValueType>::scratch (input)));
          threaded_copy (input, *in);
          std::shared_ptr <Image<ValueType> > out;

          std::unique_ptr<ProgressBar> progress;
          if (message.size()) {
            size_t axes_to_smooth = 0;
            for (std::vector<default_type>::const_iterator i = stdev.begin(); i != stdev.end(); ++i)
              if (*i)
                ++axes_to_smooth;
            progress.reset (new ProgressBar (message, axes_to_smooth + 1));
          }

          for (size_t dim = 0; dim < 3; dim++) {
            if (stdev[dim] > 0) {
              DEBUG ("creating scratch image for smoothing image along dimension " + str(dim));
              out = std::make_shared<Image<ValueType> > (Image<ValueType>::scratch (input));
              Adapter::Gaussian1D<Image<ValueType> > gaussian (*in, stdev[dim], dim, extent[dim], zero_boundary);
              threaded_copy (gaussian, *out, 0, input.ndim(), 2);
              in = out;
              if (progress)
                ++(*progress);
            }
          }
          threaded_copy (*in, output);
        }

        //! Smooth the image in place
        template <class ImageType>
        void operator() (ImageType& in_and_output)
        {
          std::unique_ptr<ProgressBar> progress;
          if (message.size()) {
            size_t axes_to_smooth = 0;
            for (std::vector<default_type>::const_iterator i = stdev.begin(); i != stdev.end(); ++i)
              if (*i)
                ++axes_to_smooth;
            progress.reset (new ProgressBar (message, axes_to_smooth + 1));
          }

          for (size_t dim = 0; dim < 3; dim++) {
            if (stdev[dim] > 0) {
              Adapter::Gaussian1DBuffered<ImageType> gaussian (in_and_output, stdev[dim], dim, extent[dim], zero_boundary);
              std::vector<size_t> axes (in_and_output.ndim(), dim);
              size_t axdim = 1;
              for (size_t i = 0; i < in_and_output.ndim(); ++i) {
                if (stride_order[i] == dim)
                  continue;
                axes[axdim++] = stride_order[i];
              }
              DEBUG ("smoothing dimension " + str(dim) + " in place with stride order: " + str(axes));
              threaded_copy (gaussian, in_and_output, axes, 1);
              if (progress)
                ++(*progress);
            }
          }
        }

      protected:
        std::vector<int> extent;
        std::vector<default_type> stdev;
        const std::vector<size_t> stride_order;
        bool zero_boundary;
    };
    //! @}
  }
}


#endif
