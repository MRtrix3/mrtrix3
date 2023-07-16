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

#ifndef __math_sphere_set_assigner_h__
#define __math_sphere_set_assigner_h__

#include "types.h"

#include "math/sphere/set/adjacency.h"


namespace MR {
  namespace Math {
    namespace Sphere {
      namespace Set {



      class Assigner : public CartesianWithAdjacency {

        public:
          template <class MatrixType>
          Assigner (const Eigen::MatrixBase<MatrixType>& dirs) :
              CartesianWithAdjacency (dirs)
          {
            initialise();
          }

          index_type operator() (const Eigen::Vector3d& dir) const;
          index_type operator() (const Eigen::Vector3d& dir, const index_type guess) const;

        private:

          vector<index_type> lookup;
          size_t resolution;


          void initialise();
          void test() const;

      };






      }
    }
  }
}

#endif

