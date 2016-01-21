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
#include "registration/transform/convert.h"
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
            ThreadKernel (Image<default_type> & deform,
                          Image<default_type> & inv_deform,
                          const size_t max_iter,
                          const default_type error_tol) :
                            deform (deform),
                            transform (inv_deform),
                            max_iter (max_iter),
                            error_tolerance (error_tol) {
                              error_tolerance = error_tolerance * error_tolerance; // to avoid a sqrt during update
            }

            void operator() (Image<default_type>& inv_deform)
            {
              Eigen::Vector3 voxel ((default_type)inv_deform.index(0), (default_type)inv_deform.index(1), (default_type)inv_deform.index(2));
              Eigen::Vector3 truth = transform.voxel2scanner * voxel;
              Eigen::Vector3 current = inv_deform.row(3);

              size_t iter = 0;
              default_type error = std::numeric_limits<default_type>::max();
              while (iter < max_iter && error > error_tolerance) {
                error = update (current, truth);
                ++iter;
              }
              inv_deform.row(3) = current;
            }

          private:

            default_type update (Eigen::Vector3& current, const Eigen::Vector3& truth)
            {
              deform.scanner (current);
              Eigen::Vector3 discrepancy = truth - deform.row(3);
              current += discrepancy;
              return discrepancy.dot (discrepancy);
            }

            Interp::Cubic<Image<default_type> > deform;
            MR::Transform transform;
            const size_t max_iter;
            default_type error_tolerance;
        };
      }


      /** \addtogroup Registration
        @{ */

          /*! Estimate the inverse of a deformation field
           * Note that the output inv_warp can be passed as either a zero field or an initial estimate
           */
          void invert_deformation (Image<default_type>& deform, Image<default_type>& inv_deform, bool is_initialised = false, size_t max_iter = 50, default_type error_tolerance = 0.01)
          {
            check_dimensions (deform, inv_deform);
            error_tolerance *= (deform.spacing(0) + deform.spacing(1) + deform.spacing(2)) / 3;

            if (!is_initialised)
              displacement2deformation (inv_deform, inv_deform);

            ThreadedLoop ("inverting warp field...", inv_deform, 0, 3)
              .run (ThreadKernel (deform, inv_deform, max_iter, error_tolerance), inv_deform);
          }

          /*! Estimate the inverse of a displacement field, output the inverse as a deformation field
           * Note that the output inv_warp can be passed as either a zero field or an initial estimate (as a deformation field)
           */
          void invert_displacement_deformation (Image<default_type>& disp, Image<default_type>& inv_deform, bool is_initialised = false, size_t max_iter = 50, default_type error_tolerance = 0.01)
          {
            auto deform_field = Image<default_type>::scratch (disp);
            Transform::displacement2deformation (disp, deform_field);

            invert_deformation (deform_field, inv_deform, is_initialised, max_iter, error_tolerance);
         }

      //! @}
    }
  }
}


#endif
