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

#ifndef __fixel_correspondence_correspondence_h__
#define __fixel_correspondence_correspondence_h__


#include "fixel/fixel.h"


#define FIXELCORRESPONDENCE_INCLUDE_ALL2ALL
//#define FIXELCORRESPONDENCE_TEST_COMBINATORICS



namespace MR {
  namespace Fixel {
    namespace Correspondence {



      // TODO Make extensive use of index_type
      using index_type = MR::Fixel::index_type;
      using dir_t = Eigen::Matrix<float, 3, 1>;
      using voxel_t = Eigen::Array<uint32_t, 3, 1>;


      constexpr unsigned int min_dirs_to_enforce_adjacency = 4;
      constexpr unsigned int max_fixels_for_no_combinatorial_warning = 6;
      constexpr unsigned int dp2cost_lookup_resolution = 1000;

      constexpr float default_in2023_alpha = 0.5f;
      constexpr float default_in2023_beta = 0.1f;
      constexpr float default_nearest_maxangle = 45.0f;

      constexpr unsigned int default_max_origins_per_target = 3;
      constexpr unsigned int default_max_objectives_per_source = 3;




    }
  }
}

#endif
