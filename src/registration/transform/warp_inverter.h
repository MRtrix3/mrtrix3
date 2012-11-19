/*
    Copyright 2012 Brain Research Institute, Melbourne, Australia

    Written by David Raffelt 19/11/12

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

#ifndef __image_filter_warp_inverter_h__
#define __image_filter_warp_inverter_h__

#include "image/info.h"
#include "image/buffer_scratch.h"
#include "image/threaded_loop.h"
#include "registration/transform/warp_composer.h"
//#include "image/adapter/info.h"
//#include "image/nav.h"
//#include "math/least_squares.h"
//#include <cmath>

namespace MR
{
  namespace Image
  {
    namespace Registration
    {

      /** \addtogroup Registration
      @{ */

      /*! Estimate the inverse of a warp field
       *
       * Typical usage:
       * \code
       * Image::Registration::WarpInvertor<Image::Buffer<float>::voxel_type> inverter (warp_vox);
       * Image::Info info (filter.info());
       * Image::Buffer<float> inv_warp_buffer (info);
       * Image::Buffer<float>::voxel_type inv_warp_vox (inv_warp_buffer);
       * inverter (warp_vox, inv_warp_vox);
       * \endcode
       */
      template <class WarpImageType>
      class WarpInverter : public ConstInfo
      {

        public:

          typename WarpImageType::voxel_type WarpVoxelType;
          typedef float value_type;

          WarpInverter (const WarpImageType& in) :
            ConstInfo (in),
            max_iter (20),
            max_error_tolerance (0.1),
            mean_error_tolerance (0.001),
            max_error_norm (std::numeric_limits<value_type>::max()),
            mean_error_norm (std::numeric_limits<value_type>::max()),
            epsilon (0.0),
            enforce_boundary_condition (true) { }


          // Note that the output can be passed as either a zero field or an initial estimate
          void operator() (WarpVoxelType& input, WarpVoxelType& output) {

            Image::Interp::Linear <WarpVoxelType> interpolator (input);
            Image::Info info (input);
            Image::BufferScratch<value_type> composed_warp (info);
            Image::BufferScratch<value_type>::voxel_type composed_warp_vox (composed_warp);
            info.set_ndim(3);
            Image::BufferScratch<value_type> scaled_norm (info);
            Image::BufferScratch<value_type>::voxel_type scaled_norm_vox;

            size_t num_of_voxels = input.dim(0) * input.dim(1) * input.dim(2);
            max_error_norm = std::numeric_limits<value_type>::max();
            mean_error_norm = std::numeric_limits<value_type>::max();
            size_t iteration = 0;

            while (iteration++ < max_iter &&
                   max_error_norm > max_error_tolerance &&
                   mean_error_norm > mean_error_tolerance) {
              WarpComposer <WarpVoxelType> composer (input, composed_warp);
              compose_kernel.add_warp (output);
              ThreadedLoop (input, 1, 0, 3).run (composer);

              max_error_norm = 0.0;
              mean_error_norm = 0.0;
            }
          }

        protected:

          size_t max_iter;
          value_type max_error_tolerance;
          value_type mean_error_tolerance;
          value_type max_error_norm;
          value_type mean_error_norm;
          value_type epsilon;
          bool  enforce_boundary_condition;
      };
      //! @}
    }
  }
}


#endif
