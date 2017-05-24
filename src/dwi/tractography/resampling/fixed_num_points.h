/* Copyright (c) 2008-2017 the MRtrix3 contributors.
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


#ifndef __dwi_tractography_resampling_fixed_num_points_h__
#define __dwi_tractography_resampling_fixed_num_points_h__


#include "dwi/tractography/resampling/resampling.h"


namespace MR {
  namespace DWI {
    namespace Tractography {
      namespace Resampling {



        class FixedNumPoints : public BaseCRTP<FixedNumPoints>
        { NOMEMALIGN

          public:
            FixedNumPoints () :
                num_points (0) { }

            FixedNumPoints (const size_t n) :
                num_points (n) { }

            bool operator() (const Streamline<>&, Streamline<>&) const override;
            bool valid() const override { return num_points; }

            void set_num_points (const size_t n) { num_points = n; }
            size_t get_num_points() const { return num_points; }

          private:
            size_t num_points;

        };



      }
    }
  }
}

#endif



