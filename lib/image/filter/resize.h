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

#ifndef __image_filter_resize_h__
#define __image_filter_resize_h__

#include "image/info.h"
#include "image/threaded_copy.h"
#include "image/filter/gaussian_smooth.h"
#include "image/filter/reslice.h"
#include "image/interp/cubic.h"
#include "image/interp/linear.h"
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
       *  An image can be up-sampled or down-sampled by defining:
       *    1) a scale factor that is applied to the image resolution (one factor for all axes, or a separate factor each axis) or
       *    2) the new voxel size (a single size for all axes, or a separate voxel size for each axis) or
       *    3) the new image resolution
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
            Resize (const InputVoxelType& in) : Info (in) { }

          void set_scale_factor (float scale) {
            set_scale_factor (std::vector<float> (3, scale));
          }

          void set_scale_factor (std::vector<float> scale) {
            if (scale.size() != 3)
              throw Exception ("a scale factor for each spatial dimension is required");

            float extent[] = {
              this->dim(0) * this->vox(0),
              this->dim(1) * this->vox(1),
              this->dim(2) * this->vox(2)
            };

            Math::Matrix<float> transform (this->transform());
            for (size_t j = 0; j < 3; ++j) {
              if (scale[j] <= 0.0)
                throw Exception ("the scale factor must be larger than zero");
              this->dim(j) = Math::ceil (this->dim(j) * scale[j]);
              float new_voxel_size = extent[j] / this->dim(j);
              for (size_t i = 0; i < 3; ++i)
                this->transform()(i,3) += 0.5 * (new_voxel_size - this->vox(j)) * transform(i,j);
              this->vox(j) = new_voxel_size;
            }
          }

          void set_voxel_size (float size) {
            std::vector <float> voxel_size (3, size);
            set_voxel_size (voxel_size);
          }

          void set_voxel_size (std::vector<float> voxel_size) {
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

          void set_resolution (std::vector<float> image_res) {
            if (image_res.size() != this->ndim())
              throw Exception ("the image resolution must be defined for all dimensions");
          }


          template <class InputVoxelType, class OutputVoxelType>
            void operator() (InputVoxelType& input, OutputVoxelType& output) {
              reslice <Image::Interp::Cubic> (input, output);
          }

      };
      //! @}
    }
  }
}


#endif
