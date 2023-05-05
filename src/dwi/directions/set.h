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

#ifndef __dwi_directions_set_h__
#define __dwi_directions_set_h__

#include "types.h"

#include "dwi/directions/adjacency.h"


namespace MR {
  namespace DWI {
    namespace Directions {



      class FastLookupSet : public CartesianWithAdjacency {

        public:
          template <class MatrixType>
          FastLookupSet (const Eigen::MatrixBase<MatrixType>& dirs) :
              CartesianWithAdjacency (dirs)
          {
            initialise();
          }

          // TODO Change to operator overload
          index_type select_direction (const Eigen::Vector3d&) const;

        private:

          vector< vector<index_type> > grid_lookup;
          unsigned int num_az_grids, num_el_grids, total_num_angle_grids;
          default_type az_grid_step, el_grid_step;
          default_type az_begin, el_begin;

          index_type select_direction_slow (const Eigen::Vector3d&) const;

          void initialise();

          size_t dir2gridindex (const Eigen::Vector3d&) const;

          void test_lookup() const;

      };



    }
  }
}

#endif

