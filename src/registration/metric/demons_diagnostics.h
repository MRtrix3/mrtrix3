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

#ifndef __registration_metric_demons_diagnostics_h__
#define __registration_metric_demons_diagnostics_h__

#include <algorithm>

#include "types.h"

namespace MR
{
  namespace Registration
  {
    namespace Metric
    {

      //! Accumulation of statistics describing one evaluation of the Demons update field
      /*! One instance is accumulated per processing thread by the metric functor,
       *  and merged into a single instance owned by the optimisation loop. */
      class DemonsDiagnostics { MEMALIGN(DemonsDiagnostics)
        public:
          DemonsDiagnostics () { clear(); }

          void clear () {
            gradient_sum = 0.0;
            gradient_max = 0.0;
            active_gradient_sum = 0.0;
            update_sum = 0.0;
            update_max = 0.0;
            voxel_count = 0;
            zeroed_count = 0;
            gradient_dominant_count = 0;
          }

          void operator+= (const DemonsDiagnostics& that) {
            gradient_sum += that.gradient_sum;
            gradient_max = std::max (gradient_max, that.gradient_max);
            active_gradient_sum += that.active_gradient_sum;
            update_sum += that.update_sum;
            update_max = std::max (update_max, that.update_max);
            voxel_count += that.voxel_count;
            zeroed_count += that.zeroed_count;
            gradient_dominant_count += that.gradient_dominant_count;
          }

          //! mean image gradient magnitude across all voxels contributing to the metric
          default_type gradient_mean () const {
            return voxel_count > 0 ? gradient_sum / static_cast<default_type> (voxel_count) : 0.0;
          }

          //! number of voxels for which a non-zero update was derived
          size_t active_count () const {
            return voxel_count - zeroed_count;
          }

          //! mean image gradient magnitude across those voxels for which a non-zero update was derived
          default_type active_gradient_mean () const {
            const size_t count = active_count();
            return count > 0 ? active_gradient_sum / static_cast<default_type> (count) : 0.0;
          }

          //! mean update magnitude across those voxels for which a non-zero update was derived
          default_type update_mean () const {
            const size_t count = voxel_count - zeroed_count;
            return count > 0 ? update_sum / static_cast<default_type> (count) : 0.0;
          }

          //! proportion of contributing voxels for which the update was explicitly set to zero
          default_type zeroed_fraction () const {
            return voxel_count > 0
                 ? static_cast<default_type> (zeroed_count) / static_cast<default_type> (voxel_count)
                 : 0.0;
          }

          //! proportion of those voxels yielding a non-zero update for which the squared gradient
          //    magnitude exceeds the intensity difference term of the Demons denominator
          default_type gradient_dominant_fraction () const {
            const size_t count = active_count();
            return count > 0
                 ? static_cast<default_type> (gradient_dominant_count) / static_cast<default_type> (count)
                 : 0.0;
          }

          default_type gradient_sum, gradient_max, active_gradient_sum;
          default_type update_sum, update_max;
          size_t voxel_count, zeroed_count, gradient_dominant_count;
      };

    }
  }
}

#endif
