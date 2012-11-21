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

#ifndef __image_filter_transform_warp_inverter_h__
#define __image_filter_transform_warp_inverter_h__

#include "image/info.h"
#include "image/buffer_scratch.h"
#include "image/interp/cubic.h"
#include "image/threaded_loop.h"
#include "image/registration/utils.h"
#include "image/transform.h"

namespace MR
{
  namespace Image
  {
    namespace Registration
    {
      namespace Transform
      {

        typedef float value_type;

        namespace {

          template <class DFVoxelType, class InvDFVoxelType>
          class ThreadKernel {

            public:
              ThreadKernel (DFVoxelType& warped_moving_positions,
                            InvDFVoxelType& inv_warp,
                            const size_t max_iter,
                            const value_type error_tol) :
                            warped_moving_positions (warped_moving_positions),
                            inv_warp (inv_warp),
                            transform (inv_warp),
                            max_iter (max_iter),
                            error_tolerance (error_tol) {
                error_tolerance = error_tolerance * error_tolerance; // to avoid a sqrt during update
              }

              void operator () (const Iterator& pos) {

                voxel_assign (inv_warp, pos, 0, 3);
                Point<float> truth = transform.voxel2scanner(pos);
                Point<float> current;
                for (inv_warp[3] = 0; inv_warp[3] < 3; ++inv_warp[3])
                  current[(size_t)inv_warp[3]] = inv_warp.value();

                size_t iter = 0;
                value_type error = std::numeric_limits<value_type>::max();
                while (iter < max_iter && error > error_tolerance) {
                  error = update (current, truth);
                  ++iter;
                }

                for (inv_warp[3] = 0; inv_warp[3] < 3; ++inv_warp[3])
                  inv_warp.value() = current[(size_t)inv_warp[3]];
              }

            private:

              value_type update (Point<float>& current, Point<float>& truth) {

                warped_moving_positions.scanner (current);
                Point<float> discrepancy;
                value_type error = 0.0;
                for (warped_moving_positions[3] = 0; warped_moving_positions[0] < 3; ++warped_moving_positions[3]) {
                  size_t dim = warped_moving_positions[3];
                  discrepancy[dim] = truth[dim] - warped_moving_positions.value();
                  error += Math::pow2 (discrepancy[dim]);
                }
                current += discrepancy;
                return error;
              }

              Interp::Cubic<DFVoxelType> warped_moving_positions;
              InvDFVoxelType inv_warp;
              Image::Transform transform;
              const size_t max_iter;
              value_type error_tolerance;
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

            template <class DFVoxelType>
              WarpInverter (const DFVoxelType& in) :
                ConstInfo (in),
                max_iter (50),
                error_tolerance (0.01),
                is_initialised (false) {
              scale_error_tolerance_by_voxel_size();
            }

            void set_max_iter (const size_t val) {
              max_iter = val;
            }

            void set_error_tolerance (const value_type val) {
              error_tolerance = val;
              scale_error_tolerance_by_voxel_size();
            }

            void set_is_initialised (const bool is_init) {
              is_initialised = is_init;
            }

            // Note that the output can be passed as either a zero field or an initial estimate
            template <class DFVoxelType, class InvDFVoxelType>
            void operator() (DFVoxelType& warp, InvDFVoxelType& inv_warp) {
              check_dimensions (warp, inv_warp);

              if (!is_initialised)
                displacement2deformation (inv_warp, inv_warp);

              BufferScratch<float> positions (*this);
              BufferScratch<float>::voxel_type positions_vox (positions);
              Image::Registration::displacement2deformation (positions_vox, positions_vox);
              Interp::Cubic<BufferScratch<float>::voxel_type> interp (positions_vox);

              BufferScratch<float> warped_positions (*this);
              BufferScratch<float>::voxel_type warped_positions_vox (warped_positions);

              LoopInOrder loop (warp, 0, 3);
              for (loop.start (warp, warped_positions_vox); loop.ok(); loop.next (warp, warped_positions_vox)) {
                Point<float> moving_pos;
                for (warp[3] = 0; warp[3] < 3; ++warp[3])
                  moving_pos[(size_t)warp[3]] = warp.value();
                interp.scanner (moving_pos);
                for (interp[3] = 0, warped_positions_vox[3] = 0; interp[3] < 3; ++interp[3], ++warped_positions_vox[3])
                  warped_positions_vox.value() = interp.value();
              }

              ThreadKernel<BufferScratch<float>::voxel_type, InvDFVoxelType> warp_updator (warped_positions_vox, inv_warp, max_iter, error_tolerance);
              ThreadedLoop (inv_warp, 1, 0, 3).run (warp_updator, "inverting warp field...");
            }

          protected:

            void scale_error_tolerance_by_voxel_size () {
              error_tolerance *= (this->vox(0) + this->vox(1) + this->vox(2)) / 3;
            }

            size_t max_iter;
            value_type error_tolerance;
            bool is_initialised;
        };
        //! @}
      }
    }
  }
}


#endif
