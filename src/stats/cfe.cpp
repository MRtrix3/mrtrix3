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

#define CFE_MERGESORT_TEST

namespace MR
{
  namespace Stats
  {
    namespace CFE
    {



      void InitMatrixFixel::add (const vector<index_type>& indices)
      {
        while (spinlock.test_and_set (std::memory_order_acquire));

        if ((*this).empty()) {
          (*this).reserve (indices.size());
          for (auto i : indices)
            (*this).emplace_back (InitMatrixElement (i));
          spinlock.clear (std::memory_order_release);
          return;
        }

#ifdef CFE_MERGESORT_TEST
        // Comprehensive test:
        // Do a memory-expensive merge, and make sure that the result of the
        //   optimised algorithm matches
        using map_type = std::map<index_type, index_type>;
        map_type map_merged;
        for (auto i : (*this))
          map_merged[i.index()] = i.value();
        for (auto i : indices) {
          const auto it = map_merged.find (i);
          if (it == map_merged.end())
            map_merged[i] = 1;
          else
            it->second++;
        }
        vector<InitMatrixElement> vector_merged;
        for (auto i : map_merged)
          vector_merged.emplace_back (InitMatrixElement (i.first, i.second));
        const vector<InitMatrixElement> original_data = (*this);
#endif

        ssize_t self_index = 0, in_index = 0;

        std::cerr << "\n\n\nCurrent contents:\n";
        for (auto i : *this)
          std::cerr << "[" << i.index() << ": " << i.value() << "] ";
        std::cerr << "\nIncoming contents:\n";
        for (auto i : indices)
          std::cerr << i << " ";
        std::cerr << "\n";

        // For anything in indices that doesn't yet appear in *this,
        //   add to this list; once completed, extend *this by the appropriate
        //   amount, and insert these into the appropriate locations
        // Save both the index of the new fixel, and the location in the array
        //   where it should be inserted to preserve sorted order
        //vector<std::pair<index_t, size_t>> new_entries;

        // Cancel that idea;
        // Construct a new vector as the sorted combination of the existing and new ones,
        //   then do a std::swap

        // TODO Needs another rethink:
        // Because memory is being re-allocated from scratch for every new fixel visitation, memory usage
        //   is actually running out faster.
        // Instead, need to continue making use of the existing allocated memory
        // Break into two passes:
        // - On first pass, increment those elements that already exist, and count the number of
        //   fixels that are not yet part of the set (but don't store them)
        // - Extend the length of the vector by as much as is required to fit the new elements
        // - On second pass, from back to front, move elements from previous back of vector to new back,
        //   inserting new elements at appropriate locations to retain sortedness of list
        const ssize_t old_size = (*this).size();
        const ssize_t in_count = indices.size();
        size_t intersection = 0;
        while (self_index < old_size && in_index < in_count) {
          if ((*this)[self_index].index() == indices[in_index]) {
            ++(*this)[self_index];
            ++self_index;
            ++in_index;
            ++intersection;
          } else if ((*this)[self_index].index() > indices[in_index]) {
            ++in_index;
          } else {
            ++self_index;
          }
        }

#ifdef CFE_MERGESORT_TEST
        const vector<InitMatrixElement> after_sweep = (*this);
#endif
        self_index = old_size - 1;
        in_index = indices.size() - 1;

        // TODO It's possible that a resize() call may always result in requesting
        //   a re-assignment of memory that exactly matches the size, which may in turn
        //   lead to memory bloat due to inability to return the old memory
        // If this occurs, iteratively calling push_back() may instead engage the
        //   memory-reservation-doubling behaviour
        (*this).resize ((*this).size() + indices.size() - intersection);
        ssize_t out_index = (*this).size() - 1;

        // TESTME
        // For each output vector location, need to determine whether it should come from copying an existing entry,
        //   or creating a new one
        //while (intersection < (*this).size()) {
        while (out_index > self_index && self_index >= 0 && in_index >= 0) {
          if ((*this)[self_index].index() == indices[in_index]) {
            (*this)[out_index] = (*this)[self_index];
            --self_index;
            --in_index;
          } else if ((*this)[self_index].index() > indices[in_index]) {
            (*this)[out_index] = (*this)[self_index];
            --self_index;
          } else {
            (*this)[out_index] = InitMatrixElement (indices[in_index]);
            --in_index;
          }
          --out_index;
        }
        while (in_index >= 0 && out_index >= 0)
          (*this)[out_index--] = InitMatrixElement (indices[in_index--]);

        ++track_count;

        std::cerr << "New contents:\n";
        for (auto i : *this)
          std::cerr << "[" << i.index() << ": " << i.value() << "] ";
        std::cerr << "\n";

#ifdef CFE_MERGESORT_TEST
        // Slow verification of contents
        assert ((*this).size() == vector_merged.size());
        for (size_t i = 0; i != (*this).size(); ++i) {
          assert ((*this)[i].index() == vector_merged[i].index());
          assert ((*this)[i].value() == vector_merged[i].value());
        }
#endif

        spinlock.clear (std::memory_order_release);
      }
/*

        vector<InitMatrixElement> combined_indices;
        combined_indices.reserve ((*this).size() + indices.size());

        std::cerr << "\n\n\nCurrent contents:\n";
        for (auto i : *this)
          std::cerr << "[" << i.index() << ": " << i.value() << "] ";
        std::cerr << "\nIncoming contents:\n";
        for (auto i : indices)
          std::cerr << i << " ";
        std::cerr << "\n";

        while (self_index < (*this).size() && in_index < indices.size()) {
          if ((*this)[self_index].index() == indices[in_index]) {
            combined_indices.emplace_back (InitMatrixElement ((*this)[self_index].index(), (*this)[self_index].value()+1));
            ++self_index;
            ++in_index;
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

        while (self_index < (*this).size()) {
          assert (in_index == indices.size());
          combined_indices.emplace_back (InitMatrixElement ((*this)[self_index++]));
        }
        while (in_index < indices.size()) {
          assert (self_index == (*this).size());
          combined_indices.emplace_back (InitMatrixElement (indices[in_index++]));
        }

        //std::swap (BaseType (*this), combined_indices);
        *this = std::move (combined_indices);
        ++track_count;

        //(*this).resize ((*this).size() + new_entries.size());

        std::cerr << "New contents:\n";
        for (auto i : *this)
          std::cerr << "[" << i.index() << ": " << i.value() << "] ";
        std::cerr << "\n";

        spinlock.clear (std::memory_order_release);
      }
*/



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
