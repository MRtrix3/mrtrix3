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


#include "fixel/matrix.h"

#include "app.h"
#include "thread_queue.h"
#include "types.h"
#include "file/path.h"
#include "file/utils.h"
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



      template <class ElementType>
      void InitFixelBase<ElementType>::add (const MappedTrack& mapped_track)
      {
        ssize_t self_index = 0, in_index = 0;

        // For anything in mapped_track that doesn't yet appear in *this,
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
        const ssize_t in_count = mapped_track.size();
        size_t intersection = 0;
        while (self_index < old_size && in_index < in_count) {
          if ((*this)[self_index].index() == mapped_track[in_index]) {
            increment ((*this)[self_index], mapped_track);
            ++self_index;
            ++in_index;
            ++intersection;
          } else if ((*this)[self_index].index() > mapped_track[in_index]) {
            ++in_index;
          } else {
            ++self_index;
          }
        }

        self_index = old_size - 1;
        in_index = mapped_track.size() - 1;

        // It's possible that a resize() call may always result in requesting
        //   a re-assignment of memory that exactly matches the size, which may in turn
        //   lead to memory bloat due to inability to return the old memory
        // If this occurs, iteratively calling push_back() may instead engage the
        //   memory-reservation-doubling behaviour
        while ((*this).size() < old_size + mapped_track.size() - intersection)
          (*this).push_back (ElementType());
        ssize_t out_index = (*this).size() - 1;

        // For each output vector location, need to determine whether it should come from copying an existing entry,
        //   or creating a new one
        while (out_index > self_index && self_index >= 0 && in_index >= 0) {
          if ((*this)[self_index].index() == mapped_track[in_index]) {
            (*this)[out_index] = (*this)[self_index];
            --self_index;
            --in_index;
          } else if ((*this)[self_index].index() > mapped_track[in_index]) {
            (*this)[out_index] = (*this)[self_index];
            --self_index;
          } else {
            (*this)[out_index] = ElementType (mapped_track[in_index], mapped_track);
            --in_index;
          }
          --out_index;
        }
        if (self_index < 0) {
          while (in_index >= 0 && out_index >= 0)
            (*this)[out_index--] = ElementType (mapped_track[in_index--], mapped_track);
        }

        // Track total number of streamlines / sum of streamline weights intersecting this fixel,
        //   independently of the extent of fixel-fixel connectivity
        increment (mapped_track);
      }

      template class InitFixelBase<InitElementUnweighted>;
      template class InitFixelBase<InitElementWeighted>;






      namespace
      {

        class TrackProcessor {

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
                             MappedTrack& out) const
            {
              using direction_type = Eigen::Vector3d;
              using SetVoxelDir = DWI::Tractography::Mapping::SetVoxelDir;

              SetVoxelDir in;
              mapper (tck, in);

              // For each voxel intersection, assign to a fixel
              out.clear();
              out.reserve (in.size());
              out.set_weight (tck.weight);
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
                  index_type closest_fixel_index = last_index;
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
                  if (closest_fixel_index != last_index && largest_dp > angular_threshold_dp)
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



        template <class MatrixType>
        class Receiver
        {
          public:
            Receiver (MatrixType& data) :
                data (data) { }
            bool operator() (const MappedTrack& in) const
            {
              try {
                for (auto f : in)
                  data[f].add (in);
                return true;
              } catch (...) {
                throw Exception ("Error assigning memory for CFE connectivity matrix");
                return false;
              }
            }
          private:
            MatrixType& data;
        };
      }



#define FIXEL_MATRIX_GENERATE_SHARED \
        auto directions_image = Fixel::find_directions_header (Path::dirname (index_image.name())).template get_image<default_type>().with_direct_io ({+2,+1}); \
        DWI::Tractography::Properties properties; \
        DWI::Tractography::Reader<float> track_file (track_filename, properties); \
        const uint32_t num_tracks = properties["count"].empty() ? 0 : to<uint32_t>(properties["count"]); \
        DWI::Tractography::Mapping::TrackLoader loader (track_file, num_tracks, "computing fixel-fixel connectivity matrix"); \
        DWI::Tractography::Mapping::TrackMapperBase mapper (index_image); \
        mapper.set_upsample_ratio (DWI::Tractography::Mapping::determine_upsample_ratio (index_image, properties, 0.333f)); \
        mapper.set_use_precise_mapping (true); \
        TrackProcessor track_processor (mapper, index_image, directions_image, fixel_mask, angular_threshold);



      InitMatrixUnweighted generate_unweighted (
          const std::string& track_filename,
          Image<index_type>& index_image,
          Image<bool>& fixel_mask,
          const float angular_threshold)
      {
        FIXEL_MATRIX_GENERATE_SHARED
        InitMatrixUnweighted connectivity_matrix (Fixel::get_number_of_fixels (index_image));
        Receiver<InitMatrixUnweighted> receiver (connectivity_matrix);
        Thread::run_queue (loader,
                           Thread::batch (DWI::Tractography::Streamline<float>()),
                           track_processor,
                           Thread::batch (MappedTrack()),
                           receiver);
        return connectivity_matrix;
      }



      InitMatrixWeighted generate_weighted (
          const std::string& track_filename,
          Image<index_type>& index_image,
          Image<bool>& fixel_mask,
          const float angular_threshold)
      {
        FIXEL_MATRIX_GENERATE_SHARED
        InitMatrixWeighted connectivity_matrix (Fixel::get_number_of_fixels (index_image));
        Receiver<InitMatrixWeighted> receiver (connectivity_matrix);
        Thread::run_queue (loader,
                           Thread::batch (DWI::Tractography::Streamline<float>()),
                           track_processor,
                           Thread::batch (MappedTrack()),
                           receiver);
        return connectivity_matrix;
      }






      template <class MatrixType>
      void Writer<MatrixType>::set_count_path (const std::string& path)
      {
        assert (!count_image.valid());
        count_image = Image<count_type>::create (path, MR::Fixel::data_header_from_nfixels (matrix.size()));
      }

      template <class MatrixType>
      void Writer<MatrixType>::set_extent_path (const std::string& path)
      {
        assert (!extent_image.valid());
        extent_image = Image<connectivity_value_type>::create (path, MR::Fixel::data_header_from_nfixels (matrix.size()));
      }



      template <class MatrixType>
      void Writer<MatrixType>::save (const std::string& path) const
      {
        if (Path::exists (path)) {
          if (Path::is_dir (path)) {
            if (!App::overwrite_files && (Path::is_file (Path::join (path, "index.mif")) ||
                                          Path::is_file (Path::join (path, "fixels.mif")) ||
                                          Path::is_file (Path::join (path, "values.mif"))))
              throw Exception ("Cannot create fixel-fixel connectivity matrix \"" + path + "\": "
                               "one or more files already exists (use -force to override)");
          } else {
            if (App::overwrite_files) {
              File::remove (path);
            } else {
              throw Exception ("Cannot create fixel-fixel connectivity matrix \"" + path + "\": Already exists as file");
            }
          }
        } else {
          File::mkdir (path);
        }

        Header index_header;
        index_header.ndim() = 4;
        index_header.size(0) = matrix.size();
        index_header.size(1) = 1;
        index_header.size(2) = 1;
        index_header.size(3) = 2;
        index_header.stride(0) = 2;
        index_header.stride(1) = 3;
        index_header.stride(2) = 4;
        index_header.stride(3) = 1;
        index_header.spacing(0) = index_header.spacing(1) = index_header.spacing(2) = 1.0;
        index_header.transform() = transform_type::Identity();
        index_header.keyval() = keyvals;
        index_header.keyval()["nfixels"] = str(matrix.size());
        index_header.datatype() = DataType::from<index_image_type>();
        index_header.datatype().set_byte_order_native();

        index_type num_connections = 0;
        {
          ProgressBar progress ("Computing number of fixel-fixel connections in matrix", matrix.size());

          for (size_t fixel_index = 0; fixel_index != matrix.size(); ++fixel_index) {
            const connectivity_value_type normalisation_factor = matrix[fixel_index].norm_factor();
            for (auto& it : matrix[fixel_index]) {
              const connectivity_value_type connectivity = normalisation_factor * it.value();
              if (connectivity >= threshold)
                ++num_connections;
            }
            ++progress;
          }
        }

        Header fixel_header;
        fixel_header.ndim() = 3;
        fixel_header.size(0) = num_connections;
        fixel_header.size(1) = 1;
        fixel_header.size(2) = 1;
        fixel_header.stride(0) = 1;
        fixel_header.stride(1) = 2;
        fixel_header.stride(2) = 3;
        fixel_header.spacing(0) = fixel_header.spacing(1) = fixel_header.spacing(2) = 1.0;
        fixel_header.transform() = transform_type::Identity();
        fixel_header.keyval() = keyvals;
        fixel_header.keyval()["nfixels"] = str(matrix.size());
        fixel_header.datatype() = DataType::from<index_type>();
        fixel_header.datatype().set_byte_order_native();

        Header value_header (fixel_header);
        value_header.datatype() = DataType::from<connectivity_value_type>();
        value_header.datatype().set_byte_order_native();

        Image<index_image_type> index_image;
        Image<index_type> fixel_image;
        Image<connectivity_value_type> value_image;

        try {
          index_image = Image<index_image_type>::create (Path::join (path, "index.mif"), index_header);
          fixel_image = Image<index_type>::create (Path::join (path, "fixels.mif"), fixel_header);
          value_image = Image<connectivity_value_type>::create (Path::join (path, "values.mif"), value_header);
        } catch (Exception& e) {
          throw Exception (e, "Unable to allocate space on filesystem for fixel-fixel connectivity matrix data");
        }

        ProgressBar progress ("Normalising and writing fixel-fixel connectivity matrix to directory \"" + path + "\"", matrix.size());
        for (size_t fixel_index = 0; fixel_index != matrix.size(); ++fixel_index) {

          const ssize_t connection_offset = fixel_image.index(0);
          index_type connection_count = 0;
          connectivity_value_type sum_connectivity = connectivity_value_type (0.0);
          const connectivity_value_type normalisation_factor = matrix[fixel_index].norm_factor();
          for (auto& it : matrix[fixel_index]) {
            const connectivity_value_type connectivity = normalisation_factor * it.value();
            if (connectivity >= threshold) {
              ++connection_count;
              sum_connectivity += connectivity;
              fixel_image.value() = it.index();
              ++fixel_image.index(0);
              value_image.value() = connectivity;
              ++value_image.index(0);
            }
          }

          index_image.index (0) = fixel_index;
          index_image.index (3) = 0; index_image.value() = uint64_t(connection_count);
          index_image.index (3) = 1; index_image.value() = connection_count ? connection_offset : uint64_t(0);

          if (count_image.valid()) {
            count_image.index(0) = fixel_index;
            count_image.value() = connection_count;
          }
          if (extent_image.valid()) {
            extent_image.index(0) = fixel_index;
            extent_image.value() = sum_connectivity;
          }

          // Force deallocation of memory used for this fixel in the generated matrix
          typename MatrixType::value_type().swap (matrix[fixel_index]);

          ++progress;
        }

      }

      template class Writer<InitMatrixUnweighted>;
      template class Writer<InitMatrixWeighted>;













      Reader::Reader (const std::string& path, const Image<bool>& mask) :
          directory (path),
          mask_image (mask)
      {
        try {
          index_image = Image<index_image_type>::open (Path::join (directory, "index.mif"));
          if (index_image.ndim() != 4)
            throw Exception ("Fixel-fixel connectivity matrix index image must be 4D");
          if (index_image.size (1) != 1 || index_image.size (2) != 1 || index_image.size (3) != 2)
            throw Exception ("Fixel-fixel connectivity matrix index image must have size Nx1x1x2");
          fixel_image = Image<fixel_index_type>::open (Path::join (directory, "fixels.mif"));
          value_image = Image<connectivity_value_type>::open (Path::join (directory, "values.mif"));
          if (value_image.size (0) != fixel_image.size (0))
            throw Exception ("Number of fixels in value image (" + str(value_image.size (0)) + ") does not match number of fixels in fixel image (" + str(fixel_image.size (0)) + ")");
          if (mask_image.valid() && size_t(mask_image.size (0)) != size())
            throw Exception ("Fixel image \"" + mask_image.name() + "\" has different number of fixels (" + str(mask_image.size (0)) + ") to fixel-fixel connectivity matrix (" + str(size()) + ")");
        } catch (Exception& e) {
          throw Exception (e, "Unable to load path \"" + directory + "\" as fixel-fixel connectivity data");
        }
      }



      Reader::Reader (const std::string& path) :
          Reader (path, Image<bool>()) { }






      NormFixel Reader::operator[] (const size_t i) const
      {
        // For thread-safety
        Image<index_image_type> index (index_image);
        Image<fixel_index_type> fixel (fixel_image);
        Image<connectivity_value_type> value (value_image);
        Image<bool> mask (mask_image);
        NormFixel result;
        if (mask.valid()) {
          mask.index(0) = i;
          if (!mask.value())
            return result;
        }
        index.index (0) = i;
        index.index (3) = 0;
        const index_image_type num_connections = index.value();
        if (!num_connections)
          return result;
        index.index (3) = 1;
        const index_image_type offset = index.value();
        connectivity_value_type sum (connectivity_value_type (0));
        fixel.index (0) = value.index (0) = offset;
        if (mask.valid()) {
          for (size_t i = 0; i != num_connections; ++i) {
            mask.index(0) = fixel.value();
            if (mask.value()) {
              result.emplace_back (NormElement (fixel.value(), value.value()));
              sum += value.value();
            }
            fixel.index (0)++; value.index (0)++;
          }
        } else {
          for (size_t i = 0; i != num_connections; ++i) {
            result.emplace_back (NormElement (fixel.value(), value.value()));
            sum += value.value();
            fixel.index (0)++; value.index (0)++;
          }
        }
        result.normalise (sum);
        return result;
      }





      size_t Reader::size (const size_t fixel) const
      {
        // For thread-safety
        Image<index_image_type> index (index_image);
        index.index (0) = fixel;
        index.index (3) = 0;
        return index.value();
      }








    }
  }
}
