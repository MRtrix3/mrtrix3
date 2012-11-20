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

namespace MR
{
  namespace Image
  {
    namespace Registration
    {

      typedef float value_type;

      namespace {

        class WarpNormCalculator {

          public:
            WarpNormCalculator (BufferScratch<value_type>::voxel_type& composed_warp,
                                BufferScratch<value_type>::voxel_type& scaled_norm_image,
                                value_type& global_sum_error_norm,
                                value_type& global_max_error_norm) :
                                  composed_warp (composed_warp),
                                  scaled_norm_image (scaled_norm_image),
                                  global_sum_error_norm (global_sum_error_norm),
                                  sum_error_norm (0.0),
                                  global_max_error_norm (global_max_error_norm),
                                  max_error_norm (0.0) {}

            ~WarpNormCalculator () {
              global_sum_error_norm += sum_error_norm;
              if (max_error_norm > global_max_error_norm)
                global_max_error_norm = max_error_norm;
            }

            void operator () (const Iterator& pos) {
              voxel_assign (composed_warp, pos, 0, 3);
              value_type scaled_norm = 0.0;

              for (size_t dim = 0; dim < 3; ++dim) {
                composed_warp[3] = dim;
                scaled_norm += Math::pow2 (composed_warp.value() / composed_warp.vox(dim));
                composed_warp.value() = -composed_warp.value();
              }

              scaled_norm = Math::sqrt (scaled_norm);
              sum_error_norm += scaled_norm;
              if (scaled_norm > max_error_norm)
                max_error_norm = scaled_norm;

              voxel_assign (scaled_norm_image, pos, 0, 3);
              scaled_norm_image.value() = scaled_norm;
            }

          private:
            BufferScratch<value_type>::voxel_type composed_warp;
            BufferScratch<value_type>::voxel_type scaled_norm_image;
            value_type& global_sum_error_norm;
            value_type sum_error_norm;
            value_type& global_max_error_norm;
            value_type max_error_norm;
        };


        template <class InvWarpVoxelType>
        class InvWarpCalculator {

          public:
            InvWarpCalculator (BufferScratch<value_type>::voxel_type& composed_warp,
                            BufferScratch<value_type>::voxel_type& scaled_norm_image,
                            InvWarpVoxelType& inv_warp,
                            const value_type max_error_norm,
                            const value_type epsilon,
                            const bool enforce_boundary_condition) :
                              composed_warp (composed_warp),
                              scaled_norm_image (scaled_norm_image),
                              inv_warp (inv_warp),
                              max_error_norm (max_error_norm),
                              epsilon (epsilon),
                              enforce_boundary_condition (enforce_boundary_condition) {}

            void operator () (const Iterator& pos) {
              voxel_assign (composed_warp, pos, 0, 3);
              voxel_assign (scaled_norm_image, pos, 0, 3);
              voxel_assign (inv_warp, pos, 0, 3);
              Point<value_type> update;
              Point<value_type> displacement;
              for (size_t dim = 0; dim < 3; ++dim) {
                composed_warp[3] = dim;
                inv_warp[3] = dim;
                update[dim] = composed_warp.value();
                displacement[dim] = inv_warp.value();
              }
              value_type scaled_norm = scaled_norm_image.value();
              if (scaled_norm > epsilon * max_error_norm)
                update *= (epsilon * max_error_norm / scaled_norm);
              update *= epsilon;
              displacement += update;
              for (size_t dim = 0; dim < 3; ++dim) {
                inv_warp[3] = dim;
                inv_warp.value() = displacement[dim];
              }

              if (enforce_boundary_condition) {
                if (pos[0] == 0 || pos[0] == (inv_warp.dim(0) - 1) ||
                    pos[1] == 0 || pos[1] == (inv_warp.dim(1) - 1) ||
                    pos[2] == 0 || pos[2] == (inv_warp.dim(2) - 1)) {
                  for (size_t dim = 0; dim < 3; ++dim) {
                    inv_warp[3] = dim;
                    inv_warp.value() = 0.0;
                  }
                }
              }
            }

          private:
            BufferScratch<value_type>::voxel_type composed_warp;
            BufferScratch<value_type>::voxel_type scaled_norm_image;
            InvWarpVoxelType inv_warp;
            const value_type max_error_norm;
            const value_type epsilon;
            const bool enforce_boundary_condition;
        };

      }


      /** \addtogroup Registration
      @{ */

      /*! Estimate the inverse of a warp field
       *
       * Typical usage:
       * \code
       * Image::Registration::WarpInvertor<Image::Buffer<float>::voxel_type> inverter (warp_vox);
       * Image::Info info (filter.info());
       * Image::Buffer<float> inv_warp_buffer (info);
       * Image::Buffer<float>::voxel_type inv_warp (inv_warp_buffer);
       * inverter (warp_vox, inv_warp);
       * \endcode
       */
      class WarpInverter : public ConstInfo
      {

        public:

          template <class WarpImageType>
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
          template <class WarpVoxelType, class InvWarpVoxelType>
            void operator() (WarpVoxelType& warp, InvWarpVoxelType& inv_warp) {

              Info info (warp);
              BufferScratch<float> composed_warp (info);
              BufferScratch<float>::voxel_type composed_warp_vox (composed_warp);
              info.set_ndim(3);
              BufferScratch<float> scaled_norm (info);
              BufferScratch<float>::voxel_type scaled_norm_vox (scaled_norm);

              size_t num_of_voxels = warp.dim(0) * warp.dim(1) * warp.dim(2);
              max_error_norm = std::numeric_limits<value_type>::max();
              mean_error_norm = std::numeric_limits<value_type>::max();
              size_t iteration = 0;

              while (iteration++ < max_iter &&
                     max_error_norm > max_error_tolerance &&
                     mean_error_norm > mean_error_tolerance) {

                CONSOLE ("iteration: " + str(iteration) + ", max_error_norm: " + str(max_error_norm) + ", mean_error_norm: " + str(mean_error_norm));

                WarpComposer <WarpVoxelType, InvWarpVoxelType, BufferScratch<float>::voxel_type> composer (warp, inv_warp, composed_warp_vox);
                ThreadedLoop (warp, 1, 0, 3).run (composer);

                mean_error_norm = 0.0;
                max_error_norm = 0.0;

                WarpNormCalculator norm_calulator (composed_warp_vox, scaled_norm_vox, mean_error_norm, max_error_norm);
                ThreadedLoop (composed_warp_vox, 1, 0, 3).run (norm_calulator);
                mean_error_norm /= static_cast<value_type> (num_of_voxels);

                epsilon = 0.5;
                if (iteration == 1)
                  epsilon = 0.75;

                InvWarpCalculator<InvWarpVoxelType> warp_updator (composed_warp_vox, scaled_norm_vox, inv_warp,
                                                 max_error_norm, epsilon, enforce_boundary_condition);
                ThreadedLoop (inv_warp, 1, 0, 3).run (warp_updator);
              }
            }

        protected:

          size_t max_iter;
          value_type max_error_tolerance;
          value_type mean_error_tolerance;
          value_type max_error_norm;
          value_type mean_error_norm;
          value_type epsilon;
          bool enforce_boundary_condition;
      };
      //! @}
    }
  }
}


#endif
