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


#ifndef __math_condition_number_h__
#define __math_condition_number_h__

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#include <Eigen/SVD>
#pragma GCC diagnostic pop

namespace MR
{
  namespace Math
  {

    template <class M>
    inline default_type condition_number (const M& data)
    {
      assert (data.rows() && data.cols());
      auto v = Eigen::JacobiSVD<M> (data).singularValues();
      return v[0] / v[v.size()-1];
    }

  }
}

#endif


