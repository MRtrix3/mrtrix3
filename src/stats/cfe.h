/*
 * Copyright (c) 2008-2018 the MRtrix3 contributors.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, you can obtain one at http://mozilla.org/MPL/2.0/
 *
 * MRtrix3 is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * For more details, see http://www.mrtrix.org/
 */


#ifndef __stats_cfe_h__
#define __stats_cfe_h__

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

        void operator() (in_column_type, out_column_type) const override;
    };



  }
}

#endif
