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


#ifndef __math_stats_types_h__
#define __math_stats_types_h__

#include "types.h"

#include <Eigen/Dense>

namespace MR
{
  namespace Math
  {
    namespace Stats
    {



      using value_type = MR::default_type;
      using matrix_type = Eigen::Matrix<value_type, Eigen::Dynamic, Eigen::Dynamic>;
      using vector_type = Eigen::Array<value_type, Eigen::Dynamic, 1>;



    }
  }
}


#endif
