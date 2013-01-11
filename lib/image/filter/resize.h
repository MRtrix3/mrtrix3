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

#include "image/info.h"
#include "image/filter/gaussian_smooth.h"
#include "image/filter/reslice.h"
#include "image/interp/nearest.h"
#include "image/interp/linear.h"
#include "image/interp/cubic.h"
#include "image/interp/sinc.h"
#include "image/buffer_scratch.h"
#include "image/copy.h"

namespace MR
{
  namespace Image
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
       * Image::BufferPreload<float> src_data (argument[0]);
       * Image::BufferPreload<float>::voxel_type src (src_data);
       * Image::Filter::Resize resize_filter (src);
       * float scale = 0.5;
       * resize_filter.set_scale_factor (scale);
       *
       * Image::Header header (src_data);
       * header.info() = resize_filter.info();
       * header.datatype() = src_data.datatype();
       *
       * Image::Buffer<float> dest_data (argument[1], src_data);
       * Image::Buffer<float>::voxel_type dest (dest_data);
       *
       * resize_filter (src, dest);
       *
       * \endcode
       */
      class Resize : public Info
      {

        public:
          template <class InputVoxelType>
            Resize (const InputVoxelType& in) : Info (in), interp_type_(2) { }


          void set_voxel_size (float size)
          {
            std::vector <float> voxel_size (3, size);
            set_voxel_size (voxel_size);
          }


          void set_voxel_size (const std::vector<float>& voxel_size)
          {
            if (voxel_size.size() != 3)
              throw Exception ("the voxel size must be defined using a value for all three dimensions.");

            Math::Matrix<float> transform (this->transform());
            for (size_t j = 0; j < 3; ++j) {
              if (voxel_size[j] <= 0.0)
                throw Exception ("the voxel size must be larger than zero");
              this->dim(j) = Math::ceil (this->dim(j) * this->vox(j) / voxel_size[j]);
              for (size_t i = 0; i < 3; ++i)
                this->transform()(i,3) += 0.5 * (voxel_size[j] - this->vox(j)) * transform(i,j);
              this->vox(j) = voxel_size[j];
            }
          }


          void set_size (const std::vector<int>& image_res)
          {
            if (image_res.size() != 3)
              throw Exception ("the image resolution must be defined for 3 spatial dimensions");
            std::vector<float> new_voxel_size (3);
            for (size_t d = 0; d < 3; ++d) {
              if (image_res[d] <= 0)
                throw Exception ("the image resolution must be larger that zero for all 3 spatial dimensions");
              new_voxel_size[d] = (this->dim(d) * this->vox(d)) / image_res[d];
            }
            set_voxel_size (new_voxel_size);
          }


          void set_scale_factor (float scale)
          {
            set_scale_factor (std::vector<float> (3, scale));
          }


          void set_scale_factor (const std::vector<float> & scale)
          {
            if (scale.size() != 3)
              throw Exception ("a scale factor for each spatial dimension is required");
            std::vector<float> new_voxel_size (3);
            for (size_t d = 0; d < 3; ++d) {
              if (scale[d] <= 0.0)
                throw Exception ("the scale factor must be larger than zero");
              new_voxel_size[d] = (this->dim(d) * this->vox(d)) / Math::ceil (this->dim(d) * scale[d]);
            }
            set_voxel_size (new_voxel_size);
          }


          void set_interp_type (int interp_type) {
            interp_type_ = interp_type;
          }


          template <class InputVoxelType, class OutputVoxelType>
            void operator() (InputVoxelType& input, OutputVoxelType& output)
            {

              bool do_smoothing = false;
              std::vector<float> stdev (input.ndim(), 0.0);
              for (unsigned int d = 0; d < 3; ++d) {
                float scale_factor = input.vox(d) / output.vox(d);
                if (scale_factor < 1.0) {
                  do_smoothing = true;
                  stdev[d] = 1.0 / (2.0 * scale_factor);
                }
              }

              if (do_smoothing) {
                Filter::GaussianSmooth<> smooth_filter (input);
                smooth_filter.set_stdev (stdev);
                BufferScratch<float> smoothed_data (input);
                BufferScratch<float>::voxel_type smoothed_voxel (smoothed_data);
                {
                  LogLevelLatch log_level (0);
                  smooth_filter (input, smoothed_voxel);
                }
                switch (interp_type_) {
                case 0:
                  reslice <Image::Interp::Nearest> (smoothed_voxel, output);
                  break;
                case 1:
                  reslice <Image::Interp::Linear> (smoothed_voxel, output);
                  break;
                case 2:
                  reslice <Image::Interp::Cubic> (smoothed_voxel, output);
                  break;
                case 3:
                  ERROR ("FIXME: sinc interpolation needs a lot of work!");
                  reslice <Image::Interp::Sinc> (smoothed_voxel, output);
                  break;
                default:
                  assert (0);
                  break;
                }
              } else {
                switch (interp_type_) {
                  case 0:
                    reslice <Image::Interp::Nearest> (input, output);
                    break;
                  case 1:
                    reslice <Image::Interp::Linear> (input, output);
                    break;
                  case 2:
                    reslice <Image::Interp::Cubic> (input, output);
                    break;
                  case 3:
                    ERROR ("FIXME: sinc interpolation needs a lot of work!");
                    reslice <Image::Interp::Sinc> (input, output);
                    break;
                  default:
                    assert (0);
                    break;
                }
              }
            }

        protected:
          int interp_type_;
      };
      //! @}
    }
  }
}


#endif
