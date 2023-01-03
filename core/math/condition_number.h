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


