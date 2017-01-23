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
#ifndef __stats_cfe_h__
#define __stats_cfe_h__

#include "math/math.h"
#include "image.h"
#include "dwi/tractography/mapping/mapper.h"

namespace MR
{
  namespace Stats
  {
    namespace CFE
    {

      typedef float value_type;
      typedef DWI::Tractography::Mapping::SetVoxelDir SetVoxelDir;


      /** \addtogroup Statistics
      @{ */


      class connectivity { MEMALIGN(connectivity)
        public:
          connectivity () : value (0.0) { }
          value_type value;
      };




      /**
       * Process each track by converting each streamline to a set of dixels, and map these to fixels.
       */
      class TrackProcessor { MEMALIGN(TrackProcessor)

        public:
          TrackProcessor (Image<int32_t>& fixel_indexer,
                          const vector<Eigen::Matrix<value_type, 3, 1> >& fixel_directions,
                          vector<uint16_t>& fixel_TDI,
                          vector<std::map<int32_t, connectivity> >& connectivity_matrix,
                          value_type angular_threshold):
                          fixel_indexer (fixel_indexer) ,
                          fixel_directions (fixel_directions),
                          fixel_TDI (fixel_TDI),
                          connectivity_matrix (connectivity_matrix) {
            angular_threshold_dp = cos (angular_threshold * (Math::pi/180.0));
          }

          bool operator() (SetVoxelDir& in)
          {
            // For each voxel tract tangent, assign to a fixel
            vector<int32_t> tract_fixel_indices;
            for (SetVoxelDir::const_iterator i = in.begin(); i != in.end(); ++i) {
              assign_pos_of (*i).to (fixel_indexer);
              fixel_indexer.index(3) = 0;
              int32_t first_index = fixel_indexer.value();
              if (first_index >= 0) {
                fixel_indexer.index(3) = 1;
                int32_t last_index = first_index + fixel_indexer.value();
                int32_t closest_fixel_index = -1;
                value_type largest_dp = 0.0;
                Eigen::Vector3f dir (i->get_dir());
                dir.normalize();
                for (int32_t j = first_index; j < last_index; ++j) {
                  value_type dp = std::abs (dir.dot (fixel_directions[j]));
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

        private:
          Image<int32_t> fixel_indexer;
          const vector<Eigen::Vector3f>& fixel_directions;
          vector<uint16_t>& fixel_TDI;
          vector<std::map<int32_t, connectivity> >& connectivity_matrix;
          value_type angular_threshold_dp;
      };




      class Enhancer { MEMALIGN(Enhancer)
        public:
          Enhancer (const vector<std::map<int32_t, connectivity> >& connectivity_map,
                    const value_type dh, const value_type E, const value_type H) :
                    connectivity_map (connectivity_map), dh (dh), E (E), H (H) { }

          value_type operator() (const value_type max_stat, const vector<value_type>& stats,
                                 vector<value_type>& enhanced_stats) const
          {
            enhanced_stats.resize (stats.size());
            std::fill (enhanced_stats.begin(), enhanced_stats.end(), 0.0);
            value_type max_enhanced_stat = 0.0;
            for (size_t fixel = 0; fixel < connectivity_map.size(); ++fixel) {
              std::map<int32_t, connectivity>::const_iterator connected_fixel;
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

        protected:
          const vector<std::map<int32_t, connectivity> >& connectivity_map;
          const value_type dh, E, H;
      };


      //! @}

    }
  }
}

#endif
