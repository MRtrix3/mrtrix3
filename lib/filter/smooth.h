/*
    Copyright 2012 Brain Research Institute, Melbourne, Australia

    Written by David Raffelt 17/02/12.

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

#ifndef __image_filter_gaussian_h__
#define __image_filter_gaussian_h__

#include "memory.h"
#include "image.h"
#include "algo/copy.h"
#include "algo/threaded_copy.h"
#include "adapter/gaussian1D.h"
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
            extent (in.ndim(), 0),
            stdev (in.ndim(), 0.0)
        {
          for (int i = 0; i < std::min (int(in.ndim()), 3); i++)
            stdev[i] = in.voxsize(i);
          datatype() = DataType::Float32;
        }

        template <class HeaderType>
        Smooth (const HeaderType& in, const std::vector<default_type>& stdev) :
            Base (in),
            extent (in.ndim(), 0),
            stdev (in.ndim())
        {
          set_stdev (stdev);
          datatype() = DataType::Float32;
        }

        //! Set the extent of smoothing kernel in voxels.
        //! This can be set as a single value for all dimensions
        //! or separate values, one for each dimension. (Default: 4 standard deviations)
        void set_extent (const std::vector<int>& new_extent)
        {
          if (new_extent.size() != 1 && new_extent.size() != this->ndim())
            throw Exception ("the number of extent elements does not correspond to the number of image dimensions");
          for (size_t i = 0; i < new_extent.size(); ++i) {
            if (!(new_extent[i] & int (1)))
              throw Exception ("expected odd number for extent");
            if (new_extent[i] < 0)
              throw Exception ("the kernel extent must be positive");
          }
          if (new_extent.size() == 1)
            for (unsigned int i = 0; i < this->ndim(); i++)
              extent[i] = new_extent[0];
          else
            extent = new_extent;
        }

        void set_stdev (default_type stdev) {
          set_stdev (std::vector<default_type> (3, stdev));
        }

        //! Set the standard deviation of the Gaussian defined in mm.
        //! This must be set as a single value to be used for the first 3 dimensions
        //! or separate values, one for each dimension. (Default: 1 voxel)
        void set_stdev (const std::vector<default_type>& std_dev)
        {
          if (std_dev.size() == 1) {
            for (unsigned int i = 0; i < 3; i++)
              stdev[i] = std_dev[0];
          } else {
            if (stdev.size() != this->ndim())
              throw Exception ("The number of stdev values supplied does not correspond to the number of dimensions");
            stdev = std_dev;
          }
          for (size_t i = 0; i < stdev.size(); ++i)
            if (stdev[i] < 0.0)
              throw Exception ("the Gaussian stdev values cannot be negative");
        }


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

          for (size_t dim = 0; dim < this->ndim(); dim++) {
            if (stdev[dim] > 0) {
              out = std::make_shared<Image<ValueType> > (Image<ValueType>::scratch (input));
              Adapter::Gaussian1D<Image<ValueType> > gaussian (*in, stdev[dim], dim, extent[dim]);
              threaded_copy (gaussian, *out);
              in = out;
              if (progress)
                ++(*progress);
            }
          }
          threaded_copy (*in, output);
        }

      protected:
        std::vector<int> extent;
        std::vector<default_type> stdev;
    };
    //! @}
  }
}


#endif
