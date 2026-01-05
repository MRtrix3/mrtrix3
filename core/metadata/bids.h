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

#ifndef __metadata_bids_h__
#define __metadata_bids_h__

#include <string>
#include <Eigen/Dense>

namespace MR {
  namespace Metadata {
    namespace BIDS {

      //! convert axis directions between formats
      /*! these helper functions convert the definition of
       *  phase-encoding direction between a 3-vector (e.g.
       *  [0 1 0] ) and a BIDS NIfTI axis identifier (e.g. 'i-')
       */
      using axis_vector_type = Eigen::Matrix<int, 3, 1>;
      std::string vector2axisid(const axis_vector_type &);
      axis_vector_type axisid2vector(const std::string &);

    }
  }
}

#endif
