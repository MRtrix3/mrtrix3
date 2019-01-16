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


#include "fixel/matrix.h"

#include "thread_queue.h"
#include "file/path.h"
#include "fixel/helpers.h"

#include "dwi/tractography/mapping/loader.h"
#include "dwi/tractography/mapping/mapper.h"
#include "dwi/tractography/mapping/voxel.h"
#include "dwi/tractography/streamline.h"

namespace MR
{
  namespace Fixel
  {
    namespace Matrix
    {



      void InitFixel::add (const vector<index_type>& indices)
      {
        if ((*this).empty()) {
          (*this).reserve (indices.size());
          for (auto i : indices)
            (*this).emplace_back (InitElement (i));
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
          (*this).push_back (InitElement());
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
            (*this)[out_index] = InitElement (indices[in_index]);
            --in_index;
          }
          --out_index;
        }
        if (self_index < 0) {
          while (in_index >= 0 && out_index >= 0)
            (*this)[out_index--] = InitElement (indices[in_index--]);
        }

        // Track total number of streamlines intersecting this fixel,
        //   independently of the extent of fixel-fixel connectivity
        ++track_count;
      }









      init_matrix_type generate (
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
                            const default_type angular_threshold) :
                mapper               (mapper),
                fixel_indexer        (fixel_indexer) ,
                fixel_directions     (fixel_directions),
                fixel_mask           (fixel_mask),
                angular_threshold_dp (std::cos (angular_threshold * (Math::pi/180.0))) { }

            bool operator() (const DWI::Tractography::Streamline<>& tck,
                             vector<index_type>& out) const
            {
              using direction_type = Eigen::Vector3;
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
                  default_type largest_dp = 0.0;
                  const direction_type dir (i.get_dir().normalized());
                  for (index_type j = first_index; j < last_index; ++j) {
                    fixel_directions.index (0) = j;
                    const default_type dp = abs (dir.dot (direction_type (fixel_directions.row (1))));
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
            const default_type angular_threshold_dp;
        };


        auto directions_image = Fixel::find_directions_header (Path::dirname (index_image.name())).template get_image<default_type>().with_direct_io ({+2,+1});
        DWI::Tractography::Properties properties;
        DWI::Tractography::Reader<float> track_file (track_filename, properties);
        const uint32_t num_tracks = properties["count"].empty() ? 0 : to<uint32_t>(properties["count"]);
        DWI::Tractography::Mapping::TrackLoader loader (track_file, num_tracks, "computing fixel-fixel connectivity matrix");
        DWI::Tractography::Mapping::TrackMapperBase mapper (index_image);
        mapper.set_upsample_ratio (DWI::Tractography::Mapping::determine_upsample_ratio (index_image, properties, 0.333f));
        mapper.set_use_precise_mapping (true);
        TrackProcessor track_processor (mapper, index_image, directions_image, fixel_mask, angular_threshold);
        init_matrix_type connectivity_matrix (Fixel::get_number_of_fixels (index_image));
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





      void normalise (init_matrix_type& initial_matrix,
                      Image<index_type>& index_image,
                      IndexRemapper index_mapper,
                      const float connectivity_threshold,
                      norm_matrix_type& normalised_matrix,
                      const float smoothing_fwhm,
                      norm_matrix_type& smoothing_matrix)
      {
        // Constants related to derivation of the smoothing matrix
        const bool do_smoothing = (smoothing_fwhm > 0.0);
        const float smooth_std_dev = smoothing_fwhm / 2.3548;
        const float gaussian_const1 = do_smoothing ? (1.0 / (smooth_std_dev *  std::sqrt (2.0 * Math::pi))) : 1.0;
        const float gaussian_const2 = 2.0 * smooth_std_dev * smooth_std_dev;


        normalised_matrix.resize (index_mapper.num_internal(), NormFixel());
        if (do_smoothing)
          smoothing_matrix.resize (index_mapper.num_internal(), NormFixel());


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
            Source (const index_type num_fixels) :
                num_fixels (num_fixels),
                counter (0),
                progress ("normalising and thresholding fixel-fixel connectivity matrix", num_fixels) { }
            bool operator() (index_type& fixel_index) {
              while (counter < num_fixels) {
                fixel_index = counter++;
                return true;
              }
              fixel_index = num_fixels;
              return false;
            }
          private:
            const index_type num_fixels;
            index_type counter;
            ProgressBar progress;
        };

        auto Sink = [&] (const index_type& input_index)
        {
          assert (input_index < initial_matrix.size());
          const index_type output_index = index_mapper.e2i (input_index);

          // Is the fixel to be "processed" outside of the provided fixel mask, and thus empty?
          if (output_index == index_mapper.invalid) {
            assert (initial_matrix[input_index].empty());
            return true;
          }

          assert (output_index < index_type(normalised_matrix.size()));

          // Here, the connectivity matrix needs to be modified to reflect the
          //   fact that fixel indices in the template fixel image may not
          //   correspond to columns in the statistical analysis
          connectivity_value_type sum_weights = 0.0;
          for (auto& it : initial_matrix[input_index]) {
            const connectivity_value_type connectivity = it.value() / connectivity_value_type (initial_matrix[input_index].count());
            if (connectivity >= connectivity_threshold) {
              normalised_matrix[output_index].push_back (NormElement (index_mapper.e2i (it.index()), connectivity));
              if (do_smoothing) {
                const default_type distance = std::sqrt (Math::pow2 (fixel_positions[input_index][0] - fixel_positions[it.index()][0]) +
                                                         Math::pow2 (fixel_positions[input_index][1] - fixel_positions[it.index()][1]) +
                                                         Math::pow2 (fixel_positions[input_index][2] - fixel_positions[it.index()][2]));
                const connectivity_value_type smoothing_weight = connectivity * gaussian_const1 * std::exp (-Math::pow2 (distance) / gaussian_const2);
                if (smoothing_weight >= connectivity_threshold) {
                  smoothing_matrix[output_index].push_back (NormElement (index_mapper.e2i (it.index()), smoothing_weight));
                  sum_weights += smoothing_weight;
                }
              }
            }
          }

          if (do_smoothing) {
            if (sum_weights) {
              // Normalise smoothing weights
              const connectivity_value_type norm_factor = connectivity_value_type(1) / sum_weights;
              for (auto i : smoothing_matrix[output_index])
                i.normalise (norm_factor);
            } else {
              // For a matrix intended for smoothing, want to give a fixel that is within the mask
              //   full self-connectivity even if it's not visited by any streamlines
              // Note however that this does not apply to the principal matrix output
              smoothing_matrix[output_index].push_back (NormElement (output_index, connectivity_value_type (1)));
            }
          }

          // Force deallocation of memory used for this fixel in the initial matrix
          InitFixel().swap (initial_matrix[input_index]);

          return true;
        };


        // Now the actual operation of the normalise_matrix() function
        Source source (index_mapper.num_external());
        Thread::run_queue (source, index_type(), Thread::multi (Sink));

        // The initial connectivity matrix should now be empty;
        //   nevertheless, wipe the memory used for the outer vector itself
        init_matrix_type().swap (initial_matrix);
      }



    }
  }
}
