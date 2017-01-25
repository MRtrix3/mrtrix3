/* Copyright (c) 2008-2017 the MRtrix3 contributors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, you can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * MRtrix is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * For more details, see http://www.mrtrix.org/.
 */


#ifndef __registration_warp_invert_h__
#define __registration_warp_invert_h__

#include "image.h"
#include "interp/linear.h"
#include "algo/threaded_loop.h"
#include "registration/warp/convert.h"
#include "transform.h"

namespace MR
{
  namespace Registration
  {
    namespace Warp
    {

      namespace {


      class DisplacementThreadKernel {

        public:
          DisplacementThreadKernel (Image<default_type> & displacement,
                        Image<default_type> & displacement_inverse,
                        const size_t max_iter,
                        const default_type error_tol) :
                          displacement (displacement),
                          transform (displacement_inverse),
                          max_iter (max_iter),
                          error_tolerance (error_tol) {}

          void operator() (Image<default_type>& displacement_inverse)
          {
            Eigen::Vector3 voxel ((default_type)displacement_inverse.index(0), (default_type)displacement_inverse.index(1), (default_type)displacement_inverse.index(2));
            Eigen::Vector3 truth = transform.voxel2scanner * voxel;
            Eigen::Vector3 current = truth + displacement_inverse.row(3);

            size_t iter = 0;
            default_type error = std::numeric_limits<default_type>::max();
            while (iter < max_iter && error > error_tolerance) {
              error = update (current, truth);
              ++iter;
            }
            displacement_inverse.row(3) = current - truth;
          }

        private:

          default_type update (Eigen::Vector3& current, const Eigen::Vector3& truth)
          {
            displacement.scanner (current);
            Eigen::Vector3 discrepancy = truth - (current + displacement.row(3));
            current += discrepancy;
            return discrepancy.dot (discrepancy);
          }

          Interp::Linear<Image<default_type> > displacement;
          MR::Transform transform;
          const size_t max_iter;
          default_type error_tolerance;
      };


        class DeformationThreadKernel {

          public:
            DeformationThreadKernel (Image<default_type> & deform,
                          Image<default_type> & inv_deform,
                          const size_t max_iter,
                          const default_type error_tol) :
                            deform (deform),
                            transform (inv_deform),
                            max_iter (max_iter),
                            error_tolerance (error_tol) {}

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

            Interp::Linear<Image<default_type> > deform;
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
          FORCE_INLINE void invert_deformation (Image<default_type>& deform_field, Image<default_type>& inv_deform_field, bool is_initialised = false, size_t max_iter = 50, default_type error_tolerance = 0.0001)
          {
            check_dimensions (deform_field, inv_deform_field);
            error_tolerance *= (deform_field.spacing(0) + deform_field.spacing(1) + deform_field.spacing(2)) / 3;

            if (!is_initialised)
              displacement2deformation (inv_deform_field, inv_deform_field);

            ThreadedLoop ("inverting warp field...", inv_deform_field, 0, 3)
              .run (DeformationThreadKernel (deform_field, inv_deform_field, max_iter, error_tolerance), inv_deform_field);
          }

          /*! Estimate the inverse of a displacement field, output the inverse as a deformation field
           * Note that the output inv_warp can be passed as either a zero field or an initial estimate (as a deformation field)
           */
          FORCE_INLINE void invert_displacement_deformation (Image<default_type>& disp, Image<default_type>& inv_deform, bool is_initialised = false, size_t max_iter = 50, default_type error_tolerance = 0.0001)
          {
            auto deform_field = Image<default_type>::scratch (disp);
            Warp::displacement2deformation (disp, deform_field);

            invert_deformation (deform_field, inv_deform, is_initialised, max_iter, error_tolerance);
         }


          /*! Estimate the inverse of a displacement field
           * Note that the output inv_warp can be passed as either a zero field or an initial estimate
           */
          FORCE_INLINE void invert_displacement (Image<default_type>& disp_field, Image<default_type>& inv_disp_field, size_t max_iter = 50, default_type error_tolerance = 0.0001)
          {
            check_dimensions (disp_field, inv_disp_field);
            error_tolerance *= (disp_field.spacing(0) + disp_field.spacing(1) + disp_field.spacing(2)) / 3;

            ThreadedLoop ("inverting displacement field...", inv_disp_field, 0, 3)
              .run (DisplacementThreadKernel (disp_field, inv_disp_field, max_iter, error_tolerance), inv_disp_field);
          }


      //! @}
    }
  }
}


#endif
