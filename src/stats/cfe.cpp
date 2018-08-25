/*
 * Copyright (c) 2008-2018 the MRtrix3 contributors.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, you can obtain one at http://mozilla.org/MPL/2.0/
 *
 * MRtrix3 is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * For more details, see http://www.mrtrix.org/
 */


#include "stats/cfe.h"

namespace MR
{
  namespace Stats
  {
    namespace CFE
    {



      void InitMatrixFixel::add (const vector<index_type>& indices)
      {
        while (spinlock.test_and_set (std::memory_order_acquire));

        size_t self_index = 0, in_index = 0;
        //int last_sign = 0;

        // For anything in indices that doesn't yet appear in *this,
        //   add to this list; once completed, extend *this by the appropriate
        //   amount, and insert these into the appropriate locations
        // Save both the index of the new fixel, and the location in the array
        //   where it should be inserted to preserve sorted order
        //vector<std::pair<index_t, size_t>> new_entries;

        // TODO Cancel that idea;
        // Construct a new vector as the sorted combination of the existing and new ones,
        //   then do a std::swap
        vector<InitMatrixElement> combined_indices;
        combined_indices.reserve ((*this).size() + indices.size());

        while (self_index < (*this).size() && in_index < indices.size()) {
          if ((*this)[self_index].index() == indices[in_index]) {
            combined_indices.emplace_back (InitMatrixElement ((*this)[self_index].index(), (*this)[self_index].value()+1));
            ++self_index;
            //++(*this)[self_index++];
            //last_sign = 0;
            //++self_index;
            //++in_index;
          } else if ((*this)[self_index].index() > indices[in_index]) {
            //if (last_sign > 0)
            //  new_entries.push_back (InitMatrixElement(indices[in_index]));
            combined_indices.emplace_back (InitMatrixElement (indices[in_index++])); // Will set count to 1 in the constructor
            //++in_index;
            //last_sign = -1;
          } else {
            combined_indices.emplace_back (InitMatrixElement ((*this)[self_index++]));
            //self_index++;
            //last_sign = 1;
          }
        }

        //std::swap (BaseType (*this), combined_indices);
        BaseType (*this) = std::move (combined_indices);
        ++track_count;

        //(*this).resize ((*this).size() + new_entries.size());

        spinlock.clear (std::memory_order_release);
      }




      TrackProcessor::TrackProcessor (const DWI::Tractography::Mapping::TrackMapperBase& mapper,
                                      Image<index_type>& fixel_indexer,
                                      const vector<direction_type>& fixel_directions,
                                      Image<bool>& fixel_mask,
                                      vector<uint16_t>& fixel_TDI,
                                      init_connectivity_matrix_type& connectivity_matrix,
                                      const value_type angular_threshold) :
                                        mapper               (mapper),
                                        fixel_indexer        (fixel_indexer) ,
                                        fixel_directions     (fixel_directions),
                                        fixel_mask           (fixel_mask),
                                        fixel_TDI            (fixel_TDI),
                                        connectivity_matrix  (connectivity_matrix),
                                        angular_threshold_dp (std::cos (angular_threshold * (Math::pi/180.0))) { }


/*
      bool TrackProcessor::operator() (const SetVoxelDir& in)
      {
        // For each voxel tract tangent, assign to a fixel
        vector<index_type> tract_fixel_indices;
        for (SetVoxelDir::const_iterator i = in.begin(); i != in.end(); ++i) {
          assign_pos_of (*i).to (fixel_indexer);
          fixel_indexer.index(3) = 0;
          const index_type num_fixels = fixel_indexer.value();
          if (num_fixels > 0) {
            fixel_indexer.index(3) = 1;
            const index_type first_index = fixel_indexer.value();
            const index_type last_index = first_index + num_fixels;
            // Note: Streamlines can still be assigned to a fixel that is outside the mask;
            //   however this will not be permitted to contribute to the matrix
            index_type closest_fixel_index = num_fixels;
            value_type largest_dp = 0.0;
            const direction_type dir (i->get_dir().normalized());
            for (index_type j = first_index; j < last_index; ++j) {
              const value_type dp = abs (dir.dot (fixel_directions[j]));
              if (dp > largest_dp) {
                largest_dp = dp;
                fixel_mask.index(0) = j;
                if (fixel_mask.value())
                  closest_fixel_index = j;
              }
            }
            if (closest_fixel_index != num_fixels && largest_dp > angular_threshold_dp) {
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
*/


      bool TrackProcessor::operator() (const DWI::Tractography::Streamline<>& tck)
      {
        DWI::Tractography::Mapping::SetVoxelDir in;
        mapper (tck, in);

        // For each voxel tract tangent, assign to a fixel
        vector<index_type> tract_fixel_indices;
        for (SetVoxelDir::const_iterator i = in.begin(); i != in.end(); ++i) {
          assign_pos_of (*i).to (fixel_indexer);
          fixel_indexer.index(3) = 0;
          const index_type num_fixels = fixel_indexer.value();
          if (num_fixels > 0) {
            fixel_indexer.index(3) = 1;
            const index_type first_index = fixel_indexer.value();
            const index_type last_index = first_index + num_fixels;
            // Note: Streamlines can still be assigned to a fixel that is outside the mask;
            //   however this will not be permitted to contribute to the matrix
            index_type closest_fixel_index = num_fixels;
            value_type largest_dp = 0.0;
            const direction_type dir (i->get_dir().normalized());
            for (index_type j = first_index; j < last_index; ++j) {
              const value_type dp = abs (dir.dot (fixel_directions[j]));
              if (dp > largest_dp) {
                largest_dp = dp;
                fixel_mask.index(0) = j;
                if (fixel_mask.value())
                  closest_fixel_index = j;
              }
            }
            if (closest_fixel_index != num_fixels && largest_dp > angular_threshold_dp) {
              tract_fixel_indices.push_back (closest_fixel_index);
              fixel_TDI[closest_fixel_index]++;
            }
          }
        }

        std::sort (tract_fixel_indices.begin(), tract_fixel_indices.end());

        try {
          for (auto f : tract_fixel_indices)
            connectivity_matrix[f].add (tract_fixel_indices);
          return true;
        } catch (...) {
          throw Exception ("Error assigning memory for CFE connectivity matrix");
          return false;
        }
      }







      Enhancer::Enhancer (const norm_connectivity_matrix_type& connectivity_matrix,
                          const value_type dh,
                          const value_type E,
                          const value_type H) :
          connectivity_matrix (connectivity_matrix),
          dh (dh),
          E (E),
          H (H) { }



      void Enhancer::operator() (in_column_type stats, out_column_type enhanced_stats) const
      {
        enhanced_stats.setZero();
        vector<NormMatrixElement>::const_iterator connected_fixel;
        for (size_t fixel = 0; fixel < connectivity_matrix.size(); ++fixel) {
          for (value_type h = this->dh; h < stats[fixel]; h +=  this->dh) {
            value_type extent = 0.0;
            for (connected_fixel = connectivity_matrix[fixel].begin(); connected_fixel != connectivity_matrix[fixel].end(); ++connected_fixel)
              if (stats[connected_fixel->index()] > h)
                extent += connected_fixel->value();
            enhanced_stats[fixel] += std::pow (extent, E) * std::pow (h, H);
          }
          enhanced_stats[fixel] *= connectivity_matrix[fixel].norm_multiplier;
        }
      }



    }
  }
}
