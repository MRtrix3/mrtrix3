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

#ifndef __registration_transform_invert_h__
#define __registration_transform_invert_h__

#include "image.h"
#include "interp/cubic.h"
#include "algo/threaded_loop.h"
#include "convert.h"
#include "transform.h"

namespace MR
{
  namespace Registration
  {
    namespace Transform
    {

      namespace {

        class ThreadKernel {

          public:
            ThreadKernel (Image<default_type> & warped_moving_positions,
                          Image<default_type> & inv_warp,
                          const size_t max_iter,
                          const default_type error_tol) :
                            warped_moving_positions (warped_moving_positions),
                            transform (inv_warp),
                            max_iter (max_iter),
                            error_tolerance (error_tol) {
                              error_tolerance = error_tolerance * error_tolerance; // to avoid a sqrt during update
            }

            void operator() (Image<default_type>& inv_warp)
            {
              Eigen::Vector3 voxel ((default_type)inv_warp.index(0), (default_type)inv_warp.index(1), (default_type)inv_warp.index(2));
              Eigen::Vector3 truth = transform.voxel2scanner * voxel;
              Eigen::Vector3 current = inv_warp.row(3);

              size_t iter = 0;
              default_type error = std::numeric_limits<default_type>::max();
              while (iter < max_iter && error > error_tolerance) {
                error = update (current, truth);
                ++iter;
              }

              inv_warp.row(3) = current;
            }

          private:

            default_type update (Eigen::Vector3& current, Eigen::Vector3& truth)
            {
              warped_moving_positions.scanner (current);
              Eigen::Vector3 moving_position = warped_moving_positions.row(3);
              Eigen::Vector3 discrepancy = truth - moving_position;
              current += discrepancy;
              return discrepancy.dot (discrepancy);
            }

            Interp::Cubic<Image<default_type> > warped_moving_positions;
            Transform transform;
            const size_t max_iter;
            default_type error_tolerance;
        };
      }


      /** \addtogroup Registration
        @{ */

          /*! Estimate the inverse of a displacement field
           * Note that the output inv_warp can be passed as either a zero field or an initial estimate
           */
          void operator() (Image<default_type>& warp, Image<default_type>& inv_warp, bool is_initialised = false, size_t max_iter = 50, default_type error_tolerance = 0.01)
          {
            check_dimensions (warp, inv_warp);

            // scale by voxel size
            error_tolerance *= (warp.spacing(0) + warp.spacing(1) + warp.spacing(2)) / 3;

            if (!is_initialised)
              displacement2deformation (inv_warp, inv_warp);

            auto positions = Image<default_type>::scratch (warp);
            displacement2deformation (positions, positions);
            Interp::Cubic<Image<default_type> > positions_interp (positions);

            auto warped_positions = Image<default_type>::scratch (warp);

            // TODO replace with displacement2deformation (positions, positions);
            for (auto i = Loop (warp, 0, 3) (warp, warped_positions); i; ++i) {
              positions_interp.scanner (warp.row(3));
              warped_positions.row(3) = positions_interp.row(3);
            }

            ThreadedLoop ("inverting warp field...", inv_warp, 0, 3)
              .run (ThreadKernel (warped_positions, inv_warp, max_iter, error_tolerance), inv_warp);
          }

      //! @}
    }
  }
}


#endif
