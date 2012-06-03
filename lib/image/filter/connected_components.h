/*
    Copyright 2012 Brain Research Institute, Melbourne, Australia

    Written by David Raffelt 08/03/12.

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

#ifndef __image_filter_connected_h__
#define __image_filter_connected_h__

#include "image/info.h"
#include "image/loop.h"
#include "image/voxel.h"
#include "image/buffer_scratch.h"
#include "math/matrix.h"


#include <iostream>

namespace MR
{
  namespace Image
  {
    namespace Filter
    {

      struct cluster {
          unsigned int label;
          unsigned int size;
      };

      bool compare_clusters (cluster i, cluster j) { return (i.size > j.size); }

      template <class MaskVoxelType>
      void compute_adjacency (MaskVoxelType & mask,
                              std::vector<Math::Matrix<float> > & adjacency_matrices,
                              std::vector<bool> & ignore_dim,
                              std::vector<std::vector<int> > & mask_indices,
                              std::vector<std::vector<uint32_t> > & adjacent_indices) {

        Image::BufferScratch<uint32_t> neigh_mask (mask);
        Image::BufferScratch<uint32_t>::voxel_type neigh_mask_vox (neigh_mask);

        // 1st pass, store mask values and their memory locations
        Image::LoopInOrder loop(mask);
        for (loop.start(mask, neigh_mask_vox); loop.ok(); loop.next(mask, neigh_mask_vox)) {
          if (mask.value() > 0.5) {
            // keep track of the index location for the second pass
            neigh_mask_vox.value() = mask_indices.size();
            std::vector<int> index(4);
            for (size_t dim = 0; dim < mask.ndim(); dim++)
              index[dim] = mask[dim];
            mask_indices.push_back (index);
          } else {
            neigh_mask_vox.value() = 0;
          }
        }

        // 2nd pass, define adjacencies
        MaskVoxelType mask_neigh (mask);
        std::vector<std::vector<int> >::iterator it;
        for (it = mask_indices.begin(); it != mask_indices.end(); ++it) {
          std::vector<uint32_t> neighbour_ptrs;
          for (size_t dim = 0; dim < mask.ndim(); dim++)
            mask_neigh[dim] = (*it)[dim];

          for (size_t dim = 0; dim < mask.ndim(); dim++) {
            if (!ignore_dim[dim]) {
              if (adjacency_matrices[dim].is_set()) {
                // for each index along this dimension, check for adjacency
                for (int i = 0; i < mask.dim(dim); i++) {
                  if (adjacency_matrices[dim]((*it)[dim], i)) {
                    mask_neigh[dim] = i;
                    if (mask_neigh.value() > 0.5) {
                      voxel_assign (neigh_mask_vox, mask_neigh);
                      neighbour_ptrs.push_back (neigh_mask_vox.value());
                    }
                  }
                }
              // we treat this dimension as having normal contiguous neighbours
              } else {
                if ((*it)[dim] > 0) {
                  mask_neigh[dim] = (*it)[dim] - 1;
                  if (mask_neigh.value() > 0.5) {
                    voxel_assign (neigh_mask_vox, mask_neigh);
                    neighbour_ptrs.push_back (neigh_mask_vox.value());
                  }
                }
                if ((*it)[dim] + 1 < mask.dim (dim)) {
                  mask_neigh[dim] = (*it)[dim] + 1;
                  if (mask_neigh.value() > 0.5) {
                    voxel_assign (neigh_mask_vox, mask_neigh);
                    neighbour_ptrs.push_back (neigh_mask_vox.value());
                  }
                }
              }
            }
            mask_neigh[dim] = (*it)[dim];
          }
          adjacent_indices.push_back (neighbour_ptrs);
        }
      }

      void agglomerate (int index,
                        std::vector<std::vector<uint32_t> > & adjacent_indices,
                        std::vector<uint32_t> & traversed,
                        uint32_t current_label,
                        uint32_t & counter) {
        counter++;
        traversed[index] = current_label;
        for (size_t n = 0; n < adjacent_indices[index].size(); n++) {
          if (!traversed[adjacent_indices[index][n]])
            agglomerate (adjacent_indices[index][n], adjacent_indices, traversed, current_label, counter);
        }
      }

      void agglomerate (int index,
                        std::vector<std::vector<uint32_t> > & adjacent_indices,
                        std::vector<uint32_t> & traversed,
                        uint32_t current_label,
                        uint32_t & counter,
                        std::vector<float> image_values,
                        float threshold) {
        counter++;
        traversed[index] = current_label;
        for (size_t n = 0; n < adjacent_indices[index].size(); n++) {
          if (image_values[adjacent_indices[index][n]] > threshold)
            if (!traversed[adjacent_indices[index][n]])
              agglomerate (adjacent_indices[index][n], adjacent_indices, traversed, current_label, counter);
        }
      }



      /** \addtogroup Filters
      @{ */

      /*! Label all connected components within a binary mask of n-dimensions.
       *  This filter will label each component in order of increasing component size
       *
       * Unless otherwise specified this filter will assume that for all dimensions
       * voxels are contiguous (in space, time or whatever you feel like). Therefore by
       * default, neighbours are defined as having contiguous indices. Alternatively
       * an adjacency matrix (that is symmetric) can be used to define neighbouring indices.
       * For example this can be used for defining adjacent neighbours in the orientation domain
       * (if the 4th dimension relates to different directions). This could also be used
       * to perform connected components within each 3D volume of a 4D image by setting
       * a zero adjacency matrix for the 4th dimension.
       *
       * Typical usage:
       * \code
       * Image::BufferPreload<float> src_data (argument[0]);
       * Image::BufferPreload<float>::voxel_type src (src_data);
       * Image::Filter::ConnectedComponents filter (src);
       *
       * Image::Header header (src_data);
       * header.info() = filter.info();
       * header.datatype() = src_data.datatype();
       *
       * Image::Buffer<float> dest_data (argument[1], src_data);
       * Image::Buffer<float>::voxel_type dest (dest_data);
       *
       * filter (src, dest);
       *
       * \endcode
       */
      class ConnectedComponents : public ConstInfo
      {
        public:

        template <class InputVoxelType>
        ConnectedComponents (const InputVoxelType& in) :
        ConstInfo (in) {
          datatype_ = DataType::Int16;
          Math::Matrix<float> empty;
          adjacency_matrices_.resize(this->ndim(), empty);
          ignore_dim_.resize(this->ndim(), false);
        }

        template <class InputVoxelType, class OutputVoxelType>
        void operator() (InputVoxelType& in, OutputVoxelType& out) {

          std::vector<std::vector<int> > mask_indices;
          std::vector<std::vector<uint32_t> > adjacent_indices;
          compute_adjacency (in, adjacency_matrices_, ignore_dim_, mask_indices, adjacent_indices);
          std::cout << "number of voxels " << mask_indices.size() << std::endl;

          std::vector<uint32_t> traversed (mask_indices.size(), 0);  // keep track of the already labelled voxels

          uint32_t current_label = 1;
          std::vector<cluster> clusters;
          for (unsigned int i = 0; i < traversed.size(); i++) {
            if (!traversed[i]) {
              cluster c;
              c.label = current_label;
              c.size = 0;
              agglomerate(i, adjacent_indices, traversed, current_label, c.size);
              clusters.push_back(c);
              current_label++;
            }
          }

          std::sort (clusters.begin(), clusters.end(), compare_clusters);

          std::vector<int> label_lookup (clusters.size(), 0);
          for (unsigned int c = 0; c < clusters.size(); c++)
            label_lookup[clusters[c].label -1] = c + 1;

          Image::LoopInOrder loop(out);
          for (loop.start(out); loop.ok(); loop.next(out))
            out.value() = 0;

          for (unsigned int i = 0; i < traversed.size(); i++) {
            for (size_t dim = 0; dim < out.ndim(); dim++)
              out[dim] = mask_indices[i][dim];
            out.value() = label_lookup[traversed[i] - 1];
          }
          std::cout << "number of clusters " << clusters.size() << std::endl;
        }

        void set_ignore_dim (bool ignore, size_t dim) {
          ignore_dim_[dim] = ignore;
        }

        void set_adjacency_matrix(Math::Matrix<float> & adj_matrix, size_t dim) {
          if (dim > this->ndim())
            throw Exception("The dimensions specified is larger than the number of input dimensions.");
          if ((int)adj_matrix.columns() != this->dim(dim) || (int)adj_matrix.rows() != this->dim(dim))
            throw Exception("The adjacency matrix size does not match the number of elements along dimension " + str(dim));

          adjacency_matrices_[dim] = adj_matrix;
        }

        protected:
          std::vector<Math::Matrix<float> > adjacency_matrices_;
          std::vector<bool> ignore_dim_;
      };
      //! @}
    }
  }
}


#endif
