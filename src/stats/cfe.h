/* Copyright (c) 2008-2023 the MRtrix3 contributors.
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

#ifndef __stats_cfe_h__
#define __stats_cfe_h__

#include "types.h"
#include "math/stats/typedefs.h"
#include "stats/enhance.h"

#include "fixel/matrix.h"

namespace MR
{
  namespace Stats
  {


    using value_type = Math::Stats::value_type;
    using direction_type = Eigen::Matrix<value_type, 3, 1>;



    class CFE : public Stats::EnhancerBase { MEMALIGN (CFE)
      public:
        CFE (const Fixel::Matrix::Reader& connectivity_matrix,
             const value_type dh, const value_type E, const value_type H, const value_type C,
             const bool norm);
        virtual ~CFE() { }

      protected:
        Fixel::Matrix::Reader matrix;
        const value_type dh, E, H, C;
        const bool normalise;

        mutable vector<value_type> h_pow_H;

        void operator() (in_column_type, out_column_type) const override;
    };



  }
}

#endif
