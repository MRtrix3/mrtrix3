/*
    Copyright 2011 Brain Research Institute, Melbourne, Australia

    Written by David Raffelt and Donald Tournier 23/07/11.

    This file is part of MRtrix.

    MRtrix is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    MRtrix is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with MRtrix.  If not, see <http://www.gnu.org/licenses/>.
 */
#ifndef __stats_cfe_h__
#define __stats_cfe_h__

#include "image/buffer_scratch.h"
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


      class connectivity {
        public:
          connectivity () : value (0.0) { }
          value_type value;
      };




      /**
       * Process each track by converting each streamline to a set of dixels, and map these to fixels.
       */
      class TrackProcessor {

        public:
          TrackProcessor (Image::BufferScratch<int32_t>& fixel_indexer,
                          const std::vector<Point<value_type> >& fixel_directions,
                          std::vector<uint16_t>& fixel_TDI,
                          std::vector<std::map<int32_t, connectivity> >& connectivity_matrix,
                          value_type angular_threshold):
                          fixel_indexer (fixel_indexer) ,
                          fixel_directions (fixel_directions),
                          fixel_TDI (fixel_TDI),
                          connectivity_matrix (connectivity_matrix) {
            angular_threshold_dp = cos (angular_threshold * (M_PI/180.0));
          }

          bool operator () (SetVoxelDir& in)
          {
            // For each voxel tract tangent, assign to a fixel
            std::vector<int32_t> tract_fixel_indices;
            for (SetVoxelDir::const_iterator i = in.begin(); i != in.end(); ++i) {
              Image::Nav::set_pos (fixel_indexer, *i);
              fixel_indexer[3] = 0;
              int32_t first_index = fixel_indexer.value();
              if (first_index >= 0) {
                fixel_indexer[3] = 1;
                int32_t last_index = first_index + fixel_indexer.value();
                int32_t closest_fixel_index = -1;
                value_type largest_dp = 0.0;
                Point<value_type> dir (i->get_dir());
                dir.normalise();
                for (int32_t j = first_index; j < last_index; ++j) {
                  value_type dp = Math::abs (dir.dot (fixel_directions[j]));
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

            for (size_t i = 0; i < tract_fixel_indices.size(); i++) {
              for (size_t j = i + 1; j < tract_fixel_indices.size(); j++) {
                connectivity_matrix[tract_fixel_indices[i]][tract_fixel_indices[j]].value++;
                connectivity_matrix[tract_fixel_indices[j]][tract_fixel_indices[i]].value++;
              }
           }


            return true;
          }

        private:
          Image::BufferScratch<int32_t>::voxel_type fixel_indexer;
          const std::vector<Point<value_type> >& fixel_directions;
          std::vector<uint16_t>& fixel_TDI;
          std::vector<std::map<int32_t, connectivity> >& connectivity_matrix;
          value_type angular_threshold_dp;
      };




      class Enhancer {
        public:
          Enhancer (const std::vector<std::map<int32_t, connectivity> >& connectivity_map,
                    const value_type dh, const value_type E, const value_type H) :
                    connectivity_map (connectivity_map), dh (dh), E (E), H (H) { }

          value_type operator() (const value_type max_stat, const std::vector<value_type>& stats,
                                 std::vector<value_type>& enhanced_stats) const
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
                enhanced_stats[fixel] += Math::pow (extent, E) * Math::pow (h, H);
              }
              if (enhanced_stats[fixel] > max_enhanced_stat)
                max_enhanced_stat = enhanced_stats[fixel];
            }

            return max_enhanced_stat;
          }

        protected:
          const std::vector<std::map<int32_t, connectivity> >& connectivity_map;
          const value_type dh, E, H;
      };


      //! @}

    }
  }
}

#endif
