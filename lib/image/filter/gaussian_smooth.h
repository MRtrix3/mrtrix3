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

#include "image/info.h"
#include "image/threaded_copy.h"
#include "image/adapter/gaussian1D.h"
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

      /*! Smooth images using a Gaussian kernel.
       *
       * Typical usage:
       * \code
       * Image::BufferPreload<float> src_data (argument[0]);
       * Image::BufferPreload<float>::voxel_type src (src_data);
       * Image::Filter::GaussianSmooth smooth_filter (src);
       *
       * std::vector<float> stdev (1);
       * stdev[0] = 2;
       * smooth_filter.set_stdev (stdev);
       *
       * Image::Header header (src_data);
       * header.info() = smooth_filter.info();
       * header.datatype() = src_data.datatype();
       *
       * Image::Buffer<float> dest_data (argument[1], src_data);
       * Image::Buffer<float>::voxel_type dest (dest_data);
       *
       * smooth_filter (src, dest);
       *
       * \endcode
       */
      class GaussianSmooth : public ConstInfo
      {

      public:
          template <class InputVoxelType>
            GaussianSmooth (const InputVoxelType& in) :
                            ConstInfo (in),
              extent_ (in.ndim(), 0),
              stdev_ (in.ndim(), 1) {
              for (unsigned int i = 0; i < in.ndim(); i++)
                stdev_[i] = in.vox(i);
          }

          template <class InputVoxelType>
            GaussianSmooth (const InputVoxelType& in,
                        const std::vector<int>& extent,
                        const std::vector<float>& stdev) :
              ConstInfo (in),
              extent_(in.ndim()),
              stdev_(in.ndim()) {
              set_extent(extent);
              set_stdev(stdev);
          }

          //! Set the extent of smoothing kernel in voxels.
          //! This can be set as a single value for all dimensions
          //! or separate values, one for each dimension. (Default: 4 standard deviations)
          void set_extent (const std::vector<int>& extent) {
            if (extent.size() == 1)
              for (unsigned int i = 0; i < this->ndim(); i++)
                extent_[i] = extent[0];
            else
              extent_ = extent;
          }

          //! Set the standard deviation of the Gaussian defined in mm.
          //! This must be set as a single value to be used for the first 3 dimensions
          //! or separate values, one for each dimension. (Default: 1 voxel)
          void set_stdev (const std::vector<float>& stdev) {
            if (stdev.size() == 1)
              for (unsigned int i = 0; i < 3; i++)
                stdev_[i] = stdev[0];
            else
              stdev_ = stdev;
          }

          template <class InputVoxelType, class OutputVoxelType>
            void operator() (InputVoxelType& input, OutputVoxelType& output) {
              if (stdev_.size() != this->ndim())
                throw Exception ("The number of stdev values supplied does not correspond to the number of dimensions");

              if (extent_.size() != this->ndim())
                throw Exception ("The number of extent values supplied does not correspond to the number of dimensions");

              RefPtr <BufferScratch<float> > in_data (new BufferScratch<float> (input));
              RefPtr <BufferScratch<float>::voxel_type> in (new BufferScratch<float>::voxel_type (*in_data));
              copy(input, *in);

              RefPtr <BufferScratch<float> > out_data;
              RefPtr <BufferScratch<float>::voxel_type> out;

              for (size_t dim = 0; dim < this->ndim(); dim++) {
                if (stdev_[dim] > 0) {
                  out_data = new BufferScratch<float> (input);
                  out = new BufferScratch<float>::voxel_type (*out_data);
                  Adapter::Gaussian1D<BufferScratch<float>::voxel_type > gaussian (*in, stdev_[dim], dim, extent_[dim]);
                  threaded_copy_with_progress_message ("Smoothing axis " + str(dim) + "...", gaussian, *out);
                  in_data = out_data;
                  in = out;
                }
              }
              Image::copy(*in, output);
            }

      protected:
          std::vector<int> extent_;
          std::vector<float> stdev_;
      };
      //! @}
    }
  }
}


#endif
