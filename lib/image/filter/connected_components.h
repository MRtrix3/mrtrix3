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

#include <stack>
#include <iostream>

namespace MR
{
  namespace Image
  {
    namespace Filter
    {

      struct cluster {
          uint32_t label;
          uint32_t size;
      };

      bool compare_clusters (cluster i, cluster j) {
        return (i.size > j.size);
      }

      void dir2adjacency (Math::Matrix<float> & dirs_el_az, float angular_threshold, Math::Matrix<float> & adjacency) {

        angular_threshold = angular_threshold * M_PI / 180.0;
        Math::Matrix<float> dirAdjacency;
        Math::Matrix<float> vert (dirs_el_az.rows(), 3);
        for (uint d = 0; d < dirs_el_az.rows(); d++) {
          vert(d,0) = cos(dirs_el_az(d,0)) * sin(dirs_el_az(d,1));
          vert(d,1) = sin(dirs_el_az(d,0)) * sin(dirs_el_az(d,1));
          vert(d,2) = cos(dirs_el_az(d,1));
        }

        dirAdjacency.resize(dirs_el_az.rows(), dirs_el_az.rows(), 0.0);
        dirAdjacency.zero();
        for (uint m = 0; m < dirs_el_az.rows(); m++) {
          for (uint n = m + 1; n < dirs_el_az.rows(); n++) {
            float angle = Math::acos(Math::dot(vert.row(m), vert.row(n)));
            if (angle > M_PI_2)
              angle = M_PI - angle;
            if (angle < angular_threshold) {
              dirAdjacency (m,n) = 1;
              dirAdjacency (n,m) = 1;
            } else {
              dirAdjacency (m,n) = 0;
              dirAdjacency (n,m) = 0;
            }
          }
        }
      }

      class AdjacencyBuilder {

        public:

          template <class MaskVoxeType>
          AdjacencyBuilder () {
            Math::Matrix<float> empty;
            adjacency_matrices_.resize(this->ndim(), empty);
            ignore_dim_.resize(this->ndim(), false);
            largest_only_ = false;
          }

          template <class MaskVoxelType>
          void compute_adjacency (MaskVoxelType & mask,
                                  std::vector<Math::Matrix<float> > & adjacency_matrices,
                                  std::vector<bool> & ignore_dim,
                                  std::vector<std::vector<int> > & mask_indices,
                                  std::vector<std::vector<uint32_t> > & adjacent_indices) {

              Image::BufferScratch<uint32_t> neigh_mask (mask);
              Image::BufferScratch<uint32_t>::voxel_type neigh_mask_vox (neigh_mask);

              // 1st pass, store mask image indices and their index in the array
              Image::LoopInOrder loop(mask);
              for (loop.start (mask, neigh_mask_vox); loop.ok(); loop.next (mask, neigh_mask_vox)) {
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

              // 2nd pass, define adjacency
              MaskVoxelType mask_neigh (mask);
              std::vector<std::vector<int> >::iterator it;
              for (it = mask_indices.begin(); it != mask_indices.end(); ++it) {
                std::vector<uint32_t> neighbour_ptrs;
                for (size_t dim = 0; dim < mask.ndim(); dim++)
                  mask_neigh[dim] = (*it)[dim];

                for (size_t dim = 0; dim < mask.ndim(); dim++) {
                  if (!ignore_dim[dim]) {
                    if (adjacency_matrices[dim].is_set()) {
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

        protected:
          std::vector<Math::Matrix<float> > adjacency_matrices_;
          std::vector<bool> ignore_dim_;
          bool largest_only_;

      };


      inline bool next_neighbour(uint32_t & node,
                                 std::vector<std::vector<uint32_t> > & adjacent_indices,
                                 std::vector<uint32_t> & labels) {

        for (size_t n = 0; n < adjacent_indices[node].size(); n++) {
          if (labels[adjacent_indices[node][n]] == 0) {
            node = adjacent_indices[node][n];
            return true;
          }
        }
        return false;
      };


      // use a non-recursive depth first search to agglomerate adjacent voxels
      inline void depth_first_search (uint32_t root,
                                      std::vector<std::vector<uint32_t> > & adjacent_indices,
                                      cluster & cluster,
                                      std::vector<uint32_t> & labels) {
        uint32_t node = root;
        std::stack<uint32_t> stack;

        while (true) {
          labels[node] = cluster.label;
          stack.push(node);
          cluster.size++;
          if (next_neighbour (node, adjacent_indices, labels)) {
            continue;
          } else {
            while (!next_neighbour (node, adjacent_indices, labels)) {
              node = stack.top();
              if (node == root)
                return;
              stack.pop();
            }
          }
        }
      }


      inline void agglomerate (std::vector<std::vector<uint32_t> > & adjacent_indices,
                               std::vector<cluster> & clusters,
                               std::vector<uint32_t> & labels) {

        labels.resize (adjacent_indices.size(), 0);
        uint32_t current_label = 1;

        for (uint32_t i = 0; i < labels.size(); i++) {
          // this node has not been already clustered
          if (labels[i] == 0) {
            cluster cluster;
            cluster.label = current_label;
            cluster.size = 0;
            depth_first_search (i, adjacent_indices, cluster, labels);
            clusters.push_back (cluster);
            current_label++;
          }
        }

        if (clusters.size() > std::numeric_limits<uint32_t>::max())
          throw Exception ("The number of clusters is larger than can be labelled with an unsigned 32bit integer.");

        std::sort (clusters.begin(), clusters.end(), compare_clusters);
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
          datatype_ = DataType::UInt32;
          Math::Matrix<float> empty;
          adjacency_matrices_.resize(this->ndim(), empty);
          ignore_dim_.resize(this->ndim(), false);
          largest_only_ = false;
        }


        template <class InputVoxelType, class OutputVoxelType>
        void operator() (InputVoxelType& in, OutputVoxelType& out) {

          std::vector<std::vector<int> > mask_indices;
          std::vector<std::vector<uint32_t> > adjacent_indices;
          compute_adjacency (in, adjacency_matrices_, ignore_dim_, mask_indices, adjacent_indices);

          std::vector<cluster> clusters;
          std::vector<uint32_t> labels;

          agglomerate (adjacent_indices, clusters, labels);

          std::vector<int> label_lookup (clusters.size(), 0);
          for (uint32_t c = 0; c < clusters.size(); c++)
            label_lookup[clusters[c].label - 1] = c + 1;

          Image::LoopInOrder loop(out);
          for (loop.start(out); loop.ok(); loop.next(out))
            out.value() = 0;

          for (uint32_t i = 0; i < adjacent_indices.size(); i++) {
            for (size_t dim = 0; dim < out.ndim(); dim++)
              out[dim] = mask_indices[i][dim];
            if (largest_only_) {
              if (label_lookup[labels[i] - 1] == 1) {
                out.value() = 1;
              }
            } else {
              out.value() = label_lookup[labels[i] - 1];
            }
          }
        }


        void set_ignore_dim (bool ignore, size_t dim) {
          ignore_dim_[dim] = ignore;
        }


        void set_adjacency_matrix (Math::Matrix<float> adj_matrix, size_t dim) {

          if (dim > this->ndim())
            throw Exception("The dimensions specified is larger than the number of input dimensions.");
          if ((int)adj_matrix.columns() != this->dim(dim) || (int)adj_matrix.rows() != this->dim(dim))
            throw Exception("The adjacency matrix size does not match the number of elements along dimension " + str(dim));
          adjacency_matrices_[dim] = adj_matrix;
        }


        void set_largest_only (bool largest_only) {
          largest_only_ = largest_only;
        }


        protected:
          std::vector<Math::Matrix<float> > adjacency_matrices_;
          std::vector<bool> ignore_dim_;
          bool largest_only_;
      };
      //! @}
    }
  }
}


#endif
