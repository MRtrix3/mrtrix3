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

#ifndef __dwi_directions_weights_h__
#define __dwi_directions_weights_h__

#include "types.h"


namespace MR {
  namespace DWI {
    namespace Directions {



      class Weights
      {
        public:
          Weights (const Eigen::MatrixXd& dirs);
          default_type operator[] (const size_t i) { assert (i < size_t(data.size())); return data[i]; }
        private:
          Eigen::Array<default_type, Eigen::Dynamic, 1> data;
      };



    }
  }
}

#endif

