/*
 * Copyright (c) 2008-2016 the MRtrix3 contributors
 * 
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/
 * 
 * MRtrix is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * 
 * For more details, see www.mrtrix.org
 * 
 */

#include "stats/cfe.h"

namespace MR
{
  namespace Stats
  {
    namespace CFE
    {



      TrackProcessor::TrackProcessor (Image<uint32_t>& fixel_indexer,
                                      const std::vector<direction_type>& fixel_directions,
                                      std::vector<uint16_t>& fixel_TDI,
                                      std::vector<std::map<int32_t, connectivity> >& connectivity_matrix,
                                      const value_type angular_threshold) :
          fixel_indexer        (fixel_indexer) ,
          fixel_directions     (fixel_directions),
          fixel_TDI            (fixel_TDI),
          connectivity_matrix  (connectivity_matrix),
          angular_threshold_dp (std::cos (angular_threshold * (Math::pi/180.0))) { }



      bool TrackProcessor::operator() (const SetVoxelDir& in)
      {
        // For each voxel tract tangent, assign to a fixel
        std::vector<int32_t> tract_fixel_indices;
        for (SetVoxelDir::const_iterator i = in.begin(); i != in.end(); ++i) {
          assign_pos_of (*i).to (fixel_indexer);
          fixel_indexer.index(3) = 0;
          uint32_t num_fibres = fixel_indexer.value();
          if (num_fibres > 0) {
            fixel_indexer.index(3) = 1;
            uint32_t first_index = fixel_indexer.value();
            uint32_t last_index = first_index + num_fibres;
            uint32_t closest_fixel_index = 0;
            value_type largest_dp = 0.0;
            const direction_type dir (i->get_dir().normalized());
            for (uint32_t j = first_index; j < last_index; ++j) {
              const value_type dp = std::abs (dir.dot (fixel_directions[j]));
              if (dp > largest_dp) {
                largest_dp = dp;
                closest_fixel_index = j;
              }
            }
            if (largest_dp > angular_threshold_dp) {
              tract_fixel_indices.push_back (closest_fixel_index);
              fixel_TDI[closest_fixel_index]++;
            }
          }
        }

        try {
          for (size_t i = 0; i < tract_fixel_indices.size(); i++) {
            for (size_t j = i + 1; j < tract_fixel_indices.size(); j++) {
              connectivity_matrix[tract_fixel_indices[i]][tract_fixel_indices[j]].value++;
              connectivity_matrix[tract_fixel_indices[j]][tract_fixel_indices[i]].value++;
            }
          }
          return true;
        } catch (...) {
          throw Exception ("Error assigning memory for CFE connectivity matrix");
          return false;
        }
      }








      Enhancer::Enhancer (const std::vector<std::map<uint32_t, connectivity> >& connectivity_map,
                          const value_type dh,
                          const value_type E,
                          const value_type H) :
          connectivity_map (connectivity_map),
          dh (dh),
          E (E),
          H (H) { }



      value_type Enhancer::operator() (const vector_type& stats, vector_type& enhanced_stats) const
      {
        enhanced_stats = vector_type::Zero (stats.size());
        value_type max_enhanced_stat = 0.0;
        for (size_t fixel = 0; fixel < connectivity_map.size(); ++fixel) {
          std::map<uint32_t, connectivity>::const_iterator connected_fixel;
          for (value_type h = this->dh; h < stats[fixel]; h +=  this->dh) {
            value_type extent = 0.0;
            for (connected_fixel = connectivity_map[fixel].begin(); connected_fixel != connectivity_map[fixel].end(); ++connected_fixel)
              if (stats[connected_fixel->first] > h)
                extent += connected_fixel->second.value;
            enhanced_stats[fixel] += std::pow (extent, E) * std::pow (h, H);
          }
          if (enhanced_stats[fixel] > max_enhanced_stat)
            max_enhanced_stat = enhanced_stats[fixel];
        }

        return max_enhanced_stat;
      }



    }
  }
}
