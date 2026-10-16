/* Copyright (c) 2008-2026 the MRtrix3 contributors.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Covered Software is provided under this License on an "as is"
 * basis, without warranty of any kind, either expressed, implied, or
 * statutory, including, without limitation, warranties that the
 * Covered Software is free of defects, merchantable, fit for a
 * particular purpose or non-infringing.
 * See the Mozilla Public License v. 2.0 for more details.
 *
 * For more details, see http://www.mrtrix.org/.
 */

#ifndef __registration_metric_linear_diagnostics_h__
#define __registration_metric_linear_diagnostics_h__

#include <algorithm>
#include <utility>

#include "types.h"

namespace MR
{
  namespace Registration
  {
    namespace Metric
    {

      //! \cond skip
      namespace {
        template <class T>
        struct LinearDiagnosticsVoid { NOMEMALIGN
          using type = void;
        };

        // value-only interpolators (as used e.g. by MeanSquaredNoGradient, or by the
        // rotation-search initialiser) carry no value_and_gradient_wrt_scanner() member;
        // linear_diagnostics_gradient_magnitude() must be usable on either kind, since
        // ThreadKernel's default operator() overload serves both.
        template <class InterpType, typename U = void>
        struct interp_has_gradient_wrt_scanner { NOMEMALIGN
          using no = int;
        };

        template <class InterpType>
        struct interp_has_gradient_wrt_scanner<InterpType, typename LinearDiagnosticsVoid<
            decltype (std::declval<InterpType&>().value_and_gradient_wrt_scanner (
                std::declval<typename InterpType::value_type&>(),
                std::declval<Eigen::Matrix<typename InterpType::value_type, 1, 3>&>()))>::type> { NOMEMALIGN
          using yes = int;
        };
      }
      //! \endcond

      //! Accumulation of the scanner-space image gradient magnitude at every voxel
      //  contributing to a linear-registration metric evaluation
      /*! One instance is accumulated per processing thread by ThreadKernel, and merged
       *  into a single instance owned by the Evaluate functor. */
      class LinearDiagnostics { MEMALIGN(LinearDiagnostics)
        public:
          LinearDiagnostics () { clear(); }

          void clear () {
            voxel_count = 0;
            im1_gradient_sum = im1_gradient_max = 0.0;
            im2_gradient_sum = im2_gradient_max = 0.0;
          }

          void operator+= (const LinearDiagnostics& that) {
            voxel_count += that.voxel_count;
            im1_gradient_sum += that.im1_gradient_sum;
            im1_gradient_max = std::max (im1_gradient_max, that.im1_gradient_max);
            im2_gradient_sum += that.im2_gradient_sum;
            im2_gradient_max = std::max (im2_gradient_max, that.im2_gradient_max);
          }

          //! mean scanner-space gradient magnitude of image 1 across contributing voxels
          default_type im1_gradient_mean () const {
            return voxel_count > 0 ? im1_gradient_sum / static_cast<default_type> (voxel_count) : 0.0;
          }

          //! mean scanner-space gradient magnitude of image 2 across contributing voxels
          default_type im2_gradient_mean () const {
            return voxel_count > 0 ? im2_gradient_sum / static_cast<default_type> (voxel_count) : 0.0;
          }

          size_t voxel_count;
          default_type im1_gradient_sum, im1_gradient_max;
          default_type im2_gradient_sum, im2_gradient_max;
      };

      //! Scanner-space gradient magnitude of \a interp at its current position
      /*! For a 4D interpolator the magnitude is averaged across the volumes along axis 3,
       *  so that a single scalar per voxel is returned irrespective of the number of
       *  contrasts or shells the image carries. Value-only interpolators (no gradient
       *  accessor) report zero: they arise on metrics for which no image gradient is
       *  ever computed (e.g. MeanSquaredNoGradient), so there is nothing to report. */
      template <class InterpType>
      inline default_type linear_diagnostics_gradient_magnitude (
          InterpType& interp, typename interp_has_gradient_wrt_scanner<InterpType>::yes = 0) {
        if (interp.ndim() == 4) {
          Eigen::Matrix<typename InterpType::value_type, Eigen::Dynamic, 1> value;
          Eigen::Matrix<typename InterpType::value_type, Eigen::Dynamic, 3> gradient;
          interp.value_and_gradient_row_wrt_scanner (value, gradient);
          return gradient.template cast<default_type>().rowwise().norm().mean();
        }
        typename InterpType::value_type value;
        Eigen::Matrix<typename InterpType::value_type, 1, 3> gradient;
        interp.value_and_gradient_wrt_scanner (value, gradient);
        return default_type (gradient.template cast<default_type>().norm());
      }

      template <class InterpType>
      inline default_type linear_diagnostics_gradient_magnitude (
          InterpType&, typename interp_has_gradient_wrt_scanner<InterpType>::no = 0) {
        return 0.0;
      }

    }
  }
}

#endif
