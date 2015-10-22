/*
    Copyright 2012 Brain Research Institute, Melbourne, Australia

    Written by David Raffelt, 10/08/2012.

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

#ifndef __image_filter_resize_h__
#define __image_filter_resize_h__

#include "filter/base.h"
#include "filter/reslice.h"
#include "filter/smooth.h"
#include "interp/nearest.h"
#include "interp/linear.h"
#include "interp/cubic.h"
#include "interp/sinc.h"
#include "image.h"
#include "algo/copy.h"

namespace MR
{
  namespace Filter
  {
    /** \addtogroup Filters
    @{ */

    /*! Resize an image
     *
     *  Note that if the image is 4D, then only the first 3 dimensions can be resized.
     *
     *  Also note that if the image is down-sampled, the appropriate smoothing is automatically applied.
     *  using Gaussian smoothing.
     *
     * Typical usage:
     * \code
     * auto input = Image<default_type>::open (argument[0]);
     * Image::Filter::Resize resize_filter (input);
     * default_type scale = 0.5;
     * resize_filter.set_scale_factor (scale);
     * auto output = Image::create (argument[1], resize_filter);
     *
     * resize_filter (src, dest);
     *
     * \endcode
     */
    class Resize : public Base
    {

      public:
        Resize (const Header& in) :
            Base (in),
            interp_type (2) { }


        void set_voxel_size (default_type size)
        {
          std::vector <default_type> voxel_size (3, size);
          set_voxel_size (voxel_size);
        }


        void set_voxel_size (const std::vector<default_type>& voxel_size)
        {
          if (voxel_size.size() != 3)
            throw Exception ("the voxel size must be defined using a value for all three dimensions.");

          Eigen::Vector3 original_extent;
          for (size_t j = 0; j < 3; ++j) {
            if (voxel_size[j] <= 0.0)
              throw Exception ("the voxel size must be larger than zero");
            original_extent[j] = axes_[j].size * axes_[j].spacing;
            axes_[j].size = std::round (axes_[j].size * axes_[j].spacing / voxel_size[j] - 0.0001); // round down at .5
            // Here we adjust the translation to ensure the image extent is centered wrt the original extent.
            // This is important when the new voxel size is not an exact multiple of the original extent
            for (size_t i = 0; i < 3; ++i)
              transform_(i,3) += 0.5 * ((voxel_size[j] - axes_[j].spacing)  + (original_extent[j] - (axes_[j].size * voxel_size[j]))) * transform_(i,j);
            axes_[j].spacing = voxel_size[j];
          }
        }


        void set_size (const std::vector<int>& image_res)
        {
          if (image_res.size() != 3)
            throw Exception ("the image resolution must be defined for 3 spatial dimensions");
          std::vector<default_type> new_voxel_size (3);
          for (size_t d = 0; d < 3; ++d) {
            if (image_res[d] <= 0)
              throw Exception ("the image resolution must be larger that zero for all 3 spatial dimensions");
            new_voxel_size[d] = (this->size(d) * this->spacing(d)) / image_res[d];
          }
          set_voxel_size (new_voxel_size);
        }


        void set_scale_factor (default_type scale)
        {
          set_scale_factor (std::vector<default_type> (3, scale));
        }


        void set_scale_factor (const std::vector<default_type> & scale)
        {
          if (scale.size() != 3)
            throw Exception ("a scale factor for each spatial dimension is required");
          std::vector<default_type> new_voxel_size (3);
          for (size_t d = 0; d < 3; ++d) {
            if (scale[d] <= 0.0)
              throw Exception ("the scale factor must be larger than zero");
            new_voxel_size[d] = (this->size(d) * this->spacing(d)) / std::ceil (this->size(d) * scale[d]);
          }
          set_voxel_size (new_voxel_size);
        }


        void set_interp_type (int type) {
          interp_type = type;
        }


        template <class InputImageType, class OutputImageType>
          void operator() (InputImageType& input, OutputImageType& output)
          {

            bool do_smoothing = false;
            std::vector<default_type> stdev (input.ndim(), 0.0);
            for (unsigned int d = 0; d < 3; ++d) {
              default_type scale_factor = (default_type)input.spacing(d) / (default_type)output.spacing(d);
              if (scale_factor < 1.0) {
                do_smoothing = true;
                stdev[d] = 1.0 / (2.0 * scale_factor);
              }
            }

            if (do_smoothing) {
              Filter::Smooth smooth_filter (input.header());
              smooth_filter.set_stdev (stdev);
              auto smoothed = Image<float>::scratch (input.header(), input.name());
              {
                LogLevelLatch log_level (0);
                smooth_filter (input, smoothed);
              }
              // No oversampling is required since the input image is pre-smoothed
              std::vector<int> oversample (3, 1.0);
              switch (interp_type) {
                case 0:
                  reslice <Interp::Nearest> (smoothed, output, Adapter::NoTransform, oversample);
                  break;
                case 1:
                  reslice <Interp::Linear> (smoothed, output, Adapter::NoTransform, oversample);
                  break;
                case 2:
                  reslice <Interp::Cubic> (smoothed, output, Adapter::NoTransform, oversample);
                  break;
                case 3:
                  reslice <Interp::Sinc> (smoothed, output, Adapter::NoTransform, oversample);
                  break;
                default:
                  assert (0);
                  break;
              }
            } else {
              switch (interp_type) {
                case 0:
                  reslice <Interp::Nearest> (input, output);
                  break;
                case 1:
                  reslice <Interp::Linear> (input, output);
                  break;
                case 2:
                  reslice <Interp::Cubic> (input, output);
                  break;
                case 3:
                  reslice <Interp::Sinc> (input, output);
                  break;
                default:
                  assert (0);
                  break;
              }
            }
          }

      protected:
        int interp_type;
    };
    //! @}
  }
}


#endif
