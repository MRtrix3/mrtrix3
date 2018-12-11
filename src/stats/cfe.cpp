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

#include "fixel/helpers.h"

#include "dwi/tractography/mapping/loader.h"
#include "dwi/tractography/mapping/mapper.h"
#include "dwi/tractography/mapping/voxel.h"
#include "dwi/tractography/streamline.h"

namespace MR
{
  namespace Stats
  {
    namespace CFE
    {



      void InitMatrixFixel::add (const vector<index_type>& indices)
      {
        if ((*this).empty()) {
          (*this).reserve (indices.size());
          for (auto i : indices)
            (*this).emplace_back (InitMatrixElement (i));
          track_count = 1;
          return;
        }

        ssize_t self_index = 0, in_index = 0;

        // For anything in indices that doesn't yet appear in *this,
        //   add to this list; once completed, extend *this by the appropriate
        //   amount, and insert these into the appropriate locations
        // Need to continue making use of the existing allocated memory
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

        self_index = old_size - 1;
        in_index = indices.size() - 1;

        // It's possible that a resize() call may always result in requesting
        //   a re-assignment of memory that exactly matches the size, which may in turn
        //   lead to memory bloat due to inability to return the old memory
        // If this occurs, iteratively calling push_back() may instead engage the
        //   memory-reservation-doubling behaviour
        while ((*this).size() < old_size + indices.size() - intersection)
          (*this).push_back (InitMatrixElement());
        ssize_t out_index = (*this).size() - 1;

        // For each output vector location, need to determine whether it should come from copying an existing entry,
        //   or creating a new one
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
        if (self_index < 0) {
          while (in_index >= 0 && out_index >= 0)
            (*this)[out_index--] = InitMatrixElement (indices[in_index--]);
        }

        // Track total number of streamlines intersecting this fixel,
        //   independently of the extent of fixel-fixel connectivity
        ++track_count;
      }










      Stats::CFE::init_connectivity_matrix_type generate_initial_matrix (
          const std::string& track_filename,
          Image<index_type>& index_image,
          Image<bool>& fixel_mask,
          const float angular_threshold)
      {

        class TrackProcessor { MEMALIGN(TrackProcessor)

          public:
            TrackProcessor (const DWI::Tractography::Mapping::TrackMapperBase& mapper,
                            Image<index_type>& fixel_indexer,
                            Image<default_type>& fixel_directions,
                            Image<bool>& fixel_mask,
                            const value_type angular_threshold) :
                mapper               (mapper),
                fixel_indexer        (fixel_indexer) ,
                fixel_directions     (fixel_directions),
                fixel_mask           (fixel_mask),
                angular_threshold_dp (std::cos (angular_threshold * (Math::pi/180.0))) { }

            bool operator() (const DWI::Tractography::Streamline<>& tck,
                             vector<index_type>& out) const
            {
              using SetVoxelDir = DWI::Tractography::Mapping::SetVoxelDir;

              SetVoxelDir in;
              mapper (tck, in);

              // For each voxel tract tangent, assign to a fixel
              out.clear();
              out.reserve (in.size());
              for (const auto& i : in) {
                assign_pos_of (i).to (fixel_indexer);
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
                  const direction_type dir (i.get_dir().normalized());
                  for (index_type j = first_index; j < last_index; ++j) {
                    fixel_directions.index (0) = j;
                    const value_type dp = abs (dir.dot (direction_type (fixel_directions.row (1))));
                    if (dp > largest_dp) {
                      largest_dp = dp;
                      fixel_mask.index(0) = j;
                      if (fixel_mask.value())
                        closest_fixel_index = j;
                    }
                  }
                  if (closest_fixel_index != num_fixels && largest_dp > angular_threshold_dp)
                    out.push_back (closest_fixel_index);
                }
              }

              // Fixel indices must be sorted prior to providing to InitMatrixFixel::add()
              std::sort (out.begin(), out.end());
              return true;
            }

          private:
            const DWI::Tractography::Mapping::TrackMapperBase& mapper;
            mutable Image<index_type> fixel_indexer;
            mutable Image<default_type> fixel_directions;
            mutable Image<bool> fixel_mask;
            const value_type angular_threshold_dp;
        };


        auto directions_image = Fixel::directions_header_from_index (index_image).template get_image<default_type>().with_direct_io ({+2,+1});
        DWI::Tractography::Properties properties;
        DWI::Tractography::Reader<float> track_file (track_filename, properties);
        const uint32_t num_tracks = properties["count"].empty() ? 0 : to<uint32_t>(properties["count"]);
        DWI::Tractography::Mapping::TrackLoader loader (track_file, num_tracks, "computing fixel-fixel connectivity matrix");
        DWI::Tractography::Mapping::TrackMapperBase mapper (index_image);
        mapper.set_upsample_ratio (DWI::Tractography::Mapping::determine_upsample_ratio (index_image, properties, 0.333f));
        mapper.set_use_precise_mapping (true);
        TrackProcessor track_processor (mapper, index_image, directions_image, fixel_mask, angular_threshold);
        init_connectivity_matrix_type connectivity_matrix (Fixel::get_number_of_fixels (index_image));
        Thread::run_queue (loader,
                           Thread::batch (DWI::Tractography::Streamline<float>()),
                           track_processor,
                           Thread::batch (vector<index_type>()),
                           // Inline lambda function for receiving streamline fixel visitations and
                           //   updating the connectivity matrix
                           [&] (const vector<index_type>& fixels)
                           {
                             try {
                               for (auto f : fixels)
                                 connectivity_matrix[f].add (fixels);
                               return true;
                             } catch (...) {
                               throw Exception ("Error assigning memory for CFE connectivity matrix");
                               return false;
                             }
                           });
        return connectivity_matrix;
      }





      void normalise_matrix (
          init_connectivity_matrix_type& initial_matrix,
          Image<index_type>& index_image,
          Image<bool>& fixel_mask,
          vector<int32_t>& index_mapping,
          const float connectivity_threshold,
          norm_connectivity_matrix_type& normalised_matrix,
          const float smoothing_fwhm,
          norm_connectivity_matrix_type& smoothing_matrix)
      {
        // Constants related to derivation of the smoothing matrix
        const bool do_smoothing = (smoothing_fwhm > 0.0);
        const float smooth_std_dev = smoothing_fwhm / 2.3548;
        const float gaussian_const1 = do_smoothing ? (1.0 / (smooth_std_dev *  std::sqrt (2.0 * Math::pi))) : 1.0;
        const float gaussian_const2 = 2.0 * smooth_std_dev * smooth_std_dev;


        // We can't allocate the output matrix (/ matrices) based on the input matrix
        // In some operational situations, all fixels are mapped from an input index
        //   to an output index, with some fixels absent. In that case (which is
        //   not necessarily equivalent to the presence or absence of a fixel mask),
        //   it is necessary to determine the maximal _output_ fixel index in order
        //   to pre-allocate these matrices.
        // TODO Replace with call to new class wrapping fixel2column & column2fixel
        uint32_t max_output_fixel_index = 0;
        for (const auto& i : index_mapping) {
          if (i > 0)
            max_output_fixel_index = std::max (max_output_fixel_index, uint32_t(i));
        }
        normalised_matrix.resize (max_output_fixel_index, NormMatrixFixel());
        if (do_smoothing)
          smoothing_matrix.resize (max_output_fixel_index, NormMatrixFixel());


        // For generating the smoothing matrix, we need to be able to quickly
        //   calculate the distance between any pair of fixels
        vector<Eigen::Vector3> fixel_positions;
        if (do_smoothing) {
          fixel_positions.resize (initial_matrix.size());
          Transform image_transform (index_image);
          for (auto i = Loop (index_image, 0, 3) (index_image); i; ++i) {
            Eigen::Vector3 vox ((default_type)index_image.index(0), (default_type)index_image.index(1), (default_type)index_image.index(2));
            vox = image_transform.voxel2scanner * vox;
            index_image.index(3) = 0;
            const index_type count = index_image.value();
            index_image.index(3) = 1;
            const index_type offset = index_image.value();
            for (size_t fixel_index = 0; fixel_index != count; ++fixel_index)
              fixel_positions[offset + fixel_index] = vox;
          }
        }


        // Define classes / functions that are going to be used for multi-threaded operation
        class Source
        { MEMALIGN(Source)
          public:
            Source (Image<bool>& mask) :
                mask (mask),
                num_fixels (mask.size (0)),
                counter (0),
                progress ("normalising and thresholding fixel-fixel connectivity matrix", num_fixels) { }
            bool operator() (size_t& fixel_index) {
              while (counter < num_fixels) {
                mask.index(0) = counter;
                ++progress;
                if (mask.value()) {
                  fixel_index = counter++;
                  return true;
                }
                ++counter;
              }
              fixel_index = num_fixels;
              return false;
            }
          private:
            Image<bool> mask;
            const size_t num_fixels;
            size_t counter;
            ProgressBar progress;
        };

        auto Sink = [&] (const size_t& input_index)
        {
          assert (input_index < initial_matrix.size());
          const int32_t output_index = index_mapping[input_index];
          assert (output_index >= 0 && output_index < int32_t(normalised_matrix.size()));

          // Here, the connectivity matrix needs to be modified to reflect the
          //   fact that fixel indices in the template fixel image may not
          //   correspond to columns in the statistical analysis
          connectivity_value_type sum_weights = 0.0;
          for (auto& it : initial_matrix[input_index]) {
            const connectivity_value_type connectivity = it.value() / connectivity_value_type (initial_matrix[input_index].count());
            if (connectivity >= connectivity_threshold) {
              normalised_matrix[output_index].push_back (Stats::CFE::NormMatrixElement (index_mapping[it.index()], connectivity));
              if (do_smoothing) {
                const value_type distance = std::sqrt (Math::pow2 (fixel_positions[input_index][0] - fixel_positions[it.index()][0]) +
                                                       Math::pow2 (fixel_positions[input_index][1] - fixel_positions[it.index()][1]) +
                                                       Math::pow2 (fixel_positions[input_index][2] - fixel_positions[it.index()][2]));
                const connectivity_value_type smoothing_weight = connectivity * gaussian_const1 * std::exp (-Math::pow2 (distance) / gaussian_const2);
                if (smoothing_weight >= connectivity_threshold) {
                  smoothing_matrix[output_index].push_back (Stats::CFE::NormMatrixElement (index_mapping[it.index()], smoothing_weight));
                  sum_weights += smoothing_weight;
                }
              }
            }
          }

          // If no streamlines traversed this fixel, connectivity matrix will be empty;
          //   let's at least inform it that it is "fully connected" to itself
          if (initial_matrix[input_index].empty()) {
            normalised_matrix[output_index].push_back (Stats::CFE::NormMatrixElement (output_index, 1.0));
            if (do_smoothing)
              smoothing_matrix[output_index].push_back (Stats::CFE::NormMatrixElement (output_index, 1.0));
          } else if (do_smoothing) {
            // Normalise smoothing weights
            const connectivity_value_type norm_factor = connectivity_value_type(1.0) / sum_weights;
            for (auto i : smoothing_matrix[output_index])
              i.normalise (norm_factor);
          }

          // Force deallocation of memory used for this fixel in the initial matrix
          Stats::CFE::InitMatrixFixel().swap (initial_matrix[input_index]);

          return true;
        };


        // Now the actual operation of the normalise_matrix() function
        Source source (fixel_mask);
        Thread::run_queue (source, size_t(), Thread::multi (Sink));

        // The initial connectivity matrix should now be empty;
        //   nevertheless, wipe the memory used for the outer vector itself
        Stats::CFE::init_connectivity_matrix_type().swap (initial_matrix);
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
