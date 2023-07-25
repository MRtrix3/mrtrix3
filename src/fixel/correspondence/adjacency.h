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

#include "dwi/directions/set.h"

#include "fixel/correspondence/correspondence.h"
#include "fixel/correspondence/fixel.h"


namespace MR {
  namespace Fixel {
    namespace Correspondence {





      // Class that assists in classifying impermissible mappings based on fixel adjacency
      // If the number of fixels is fewer than 4 / 5, then any mapping will be permitted;
      //   not only because all fixels are adjacent by definition, but also because the convex set
      //   algorithm requires that at least 4 directions be present in order to initialise
      class Adjacency
      {

        public:
          Adjacency (const vector<Correspondence::Fixel>& fixels)
          {
            if (fixels.size() < min_dirs_to_enforce_adjacency)
              return;
            Eigen::Matrix<float, Eigen::Dynamic, Eigen::Dynamic> temp (fixels.size(), 3);
            for (size_t i = 0; i != fixels.size(); ++i)
              temp.row(i) = fixels[i].dir();
            dirs.reset (new DWI::Directions::Set (temp));
          }

          // Are two directions adjacent to one another?
          bool operator() (const size_t i, const size_t j) const
          {
            if (!dirs) return true;
            assert (i < dirs->size());
            assert (j < dirs->size());
            return dirs->dirs_are_adjacent (i, j);
          }

          // Is a specific set of source fixels permissible?
          // For all fixels, at least one of the other fixels in the set must be present in the adjacency set
          bool operator() (const vector<uint32_t>& indices) const
          {
            if (!dirs || indices.size() < 2)
              return true;
            for (const auto i : indices) {
              const auto& list (dirs->get_adj_dirs (i));
              // For each direction that is adjacent to "i", search through "indices" in pursuit of a match;
              //   if *none* of those adjacent directions are also part of set "indices" (i.e. iterator != end, indicating a hit),
              //   then this fixel is disconnected from the rest of the fixel set in "indices", and so should be rejected as a candidate
              if (std::none_of (list.begin(),
                                list.end(),
                                [&] (const DWI::Directions::index_type j)
                                {
                                  return std::find (indices.begin(), indices.end(), j) != indices.end();
                                }))
                return false;
            }
            return true;
          }

        private:
          std::unique_ptr<DWI::Directions::Set> dirs;

      };



    }
  }
}