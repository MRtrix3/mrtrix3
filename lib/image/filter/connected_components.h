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
#include "image/nav.h"
#include "math/matrix.h"

#include <stack>
#include <iostream>

namespace MR
{
  namespace Image
  {
    namespace Filter
    {

      struct cluster
      {
        uint32_t label;
        uint32_t size;
      };


      inline bool compare_clusters (cluster i, cluster j)
      {
        return (i.size > j.size);
      }


      class Connector {

        public:
          Connector (bool do_26_connectivity) :
            do_26_connectivity_ (do_26_connectivity) {
          }


          // Perform connected components on the mask.
          const std::vector<std::vector<int> > & run (std::vector<cluster> & clusters,
                                                      std::vector<uint32_t> & labels) const {
            labels.resize (adjacent_indices_.size(), 0);
            uint32_t current_label = 1;
            for (uint32_t i = 0; i < labels.size(); i++) {
              // this node has not been already clustered
              if (labels[i] == 0) {
                cluster cluster;
                cluster.label = current_label;
                cluster.size = 0;
                depth_first_search (i, cluster, labels);
                clusters.push_back (cluster);
                current_label++;
              }
            }
            if (clusters.size() > std::numeric_limits<uint32_t>::max())
              throw Exception ("The number of clusters is larger than can be labelled with an unsigned 32bit integer.");
            return mask_indices_;
          }


          // Perform connected components on data with the defined threshold. Assumes adjacency is the same as the mask.
          void run (std::vector<cluster> & clusters,
                    std::vector<uint32_t> & labels,
                    const std::vector<float> & data,
                    const float threshold) const {
            labels.resize (adjacent_indices_.size(), 0);
            uint32_t current_label = 1;
            for (uint32_t i = 0; i < labels.size(); i++) {
              // this node has not been already clustered and is above threshold
              if (labels[i] == 0 && data[i] > threshold) {
                cluster cluster;
                cluster.label = current_label;
                cluster.size = 0;
                depth_first_search (i, cluster, labels, data, threshold);
                clusters.push_back (cluster);
                current_label++;
              }
            }
            if (clusters.size() > std::numeric_limits<uint32_t>::max())
              throw Exception ("The number of clusters is larger than can be labelled with an unsigned 32bit integer.");
          }


          void set_directions (Math::Matrix<float> & dirs_az_el, float angular_threshold) {
            angular_threshold = angular_threshold * M_PI / 180.0;
            Math::Matrix<float> vert (dirs_az_el.rows(), 3);
            for (uint d = 0; d < dirs_az_el.rows(); d++) {
              vert(d,0) = sin(dirs_az_el(d,1)) * cos(dirs_az_el(d,0));
              vert(d,1) = sin(dirs_az_el(d,1)) * sin(dirs_az_el(d,0));
              vert(d,2) = cos(dirs_az_el(d,1));
            }
            dir_adjacency_matrix_.resize(dirs_az_el.rows(), dirs_az_el.rows(), 0.0);
            for (uint m = 0; m < dirs_az_el.rows(); m++) {
              for (uint n = m + 1; n < dirs_az_el.rows(); n++) {
                float angle = Math::acos(Math::dot(vert.row(m), vert.row(n)));
                if (angle > M_PI_2)
                  angle = M_PI - angle;
                if (angle < angular_threshold) {
                  dir_adjacency_matrix_ (m,n) = 1;
                  dir_adjacency_matrix_ (n,m) = 1;
                } else {
                  dir_adjacency_matrix_ (m,n) = 0;
                  dir_adjacency_matrix_ (n,m) = 0;
                }
              }
            }
          }


          void set_ignore_dim (bool ignore, size_t dim) {
            ignore_dim_[dim] = ignore;
          }

          template <class MaskVoxelType>
          const std::vector<std::vector<int> > & precompute_adjacency (MaskVoxelType & mask) {
            ignore_dim_.resize (mask.ndim(), false);
            Image::BufferScratch<uint32_t> index_data (mask);
            Image::BufferScratch<uint32_t>::voxel_type index_image (index_data);
            // 1st pass, store mask image indices and their index in the array
            Image::LoopInOrder loop (mask);
            for (loop.start (mask, index_image); loop.ok(); loop.next (mask, index_image)) {
              if (mask.value() >= 0.5) {
                // For each voxel, store the index within mask_indices for 2nd pass
                index_image.value() = mask_indices_.size();
                std::vector<int> index(4);
                for (size_t dim = 0; dim < mask.ndim(); dim++)
                  index[dim] = mask[dim];
                mask_indices_.push_back (index);
              } else {
                index_image.value() = 0;
              }
            }
            // Here we pre-compute the offsets for our neighbours in 3D space
            std::vector<std::vector<int> > neighbour_offsets;
            for (int x = -1; x <= 1; x++) {
              for (int y = -1; y <= 1; y++) {
                for (int z = -1; z <= 1; z++) {
                  if (x != 0 || y != 0 || z != 0) {
                    int temp = abs(x) + abs(y) + abs(z);
                    if (!do_26_connectivity_ && temp > 1)
                      continue;
                    if (abs(x) && ignore_dim_[0])
                      continue;
                    if (abs(y) && ignore_dim_[1])
                      continue;
                    if (abs(z) && ignore_dim_[2])
                      continue;
                    std::vector<int> offset(3);
                    offset[0] = x;
                    offset[1] = y;
                    offset[2] = z;
                    neighbour_offsets.push_back(offset);
                  }
                }
              }
            }
            // 2nd pass, define adjacency
            MaskVoxelType mask_neigh (mask);
            std::vector<std::vector<int> >::iterator it;
            for (it = mask_indices_.begin(); it != mask_indices_.end(); ++it) {
              std::vector<uint32_t> neighbour_indices;
              if (mask.ndim() == 4)
                mask_neigh[3] = (*it)[3];
              for (size_t n = 0; n < neighbour_offsets.size(); ++n) {
                for (size_t dim = 0; dim < 3; dim++)
                  mask_neigh[dim] = (*it)[dim] + neighbour_offsets[n][dim];
                if (Image::Nav::within_bounds(mask_neigh)) {
                  if (mask_neigh.value() >= 0.5) {
                    voxel_assign (index_image, mask_neigh);
                    neighbour_indices.push_back (index_image.value());
                  }
                }
              }
              // here we handle the 4th dimension
              if (mask.ndim() == 4 && !ignore_dim_[3]) {
                for (size_t dim = 0; dim < 3; dim++)
                  mask_neigh[dim] = (*it)[dim];
                if (dir_adjacency_matrix_.is_set()) {
                  for (int i = 0; i < mask.dim(3); i++) {
                    if (dir_adjacency_matrix_((*it)[3], i)) {
                      mask_neigh[3] = i;
                      if (mask_neigh.value() > 0.5) {
                        voxel_assign (index_image, mask_neigh);
                        neighbour_indices.push_back (index_image.value());
                      }
                    }
                  }
                } else {
                  if ((*it)[3] > 0) {  // boundary check
                    mask_neigh[3] = (*it)[3] - 1;
                    if (mask_neigh.value() > 0.5) {
                      voxel_assign (index_image, mask_neigh);
                      neighbour_indices.push_back (index_image.value());
                    }
                  }
                  if ((*it)[3] + 1 < mask.dim (3)) { // boundary check
                    mask_neigh[3] = (*it)[3] + 1;
                    if (mask_neigh.value() > 0.5) {
                      voxel_assign (index_image, mask_neigh);
                      neighbour_indices.push_back (index_image.value());
                    }
                  }
                }
              }
              adjacent_indices_.push_back(neighbour_indices);
            }
            return mask_indices_;
          }


          bool next_neighbour(uint32_t & node, std::vector<uint32_t> & labels) const {
            for (size_t n = 0; n < adjacent_indices_[node].size(); n++) {
              if (labels[adjacent_indices_[node][n]] == 0) {
                node = adjacent_indices_[node][n];
                return true;
              }
            }
            return false;
          }


          bool next_neighbour(uint32_t & node,
                              std::vector<uint32_t> & labels,
                              const std::vector<float> & data,
                              const float threshold) const {
            for (size_t n = 0; n < adjacent_indices_[node].size(); n++) {
              if (labels[adjacent_indices_[node][n]] == 0 && data[adjacent_indices_[node][n]] > threshold) {
                node = adjacent_indices_[node][n];
                return true;
              }
            }
            return false;
          }


          // use a non-recursive depth first search to agglomerate adjacent voxels
          void depth_first_search (uint32_t root,
                                   cluster & cluster,
                                   std::vector<uint32_t> & labels) const {
            uint32_t node = root;
            std::stack<uint32_t> stack;
            while (true) {
              labels[node] = cluster.label;
              stack.push (node);
              cluster.size++;
              if (next_neighbour (node, labels)) {
                continue;
              } else {
                do {
                  if (stack.top() == root)
                    return;
                  stack.pop();
                  node = stack.top();
                } while (!next_neighbour (node, labels));
              }
            }
          }

          // use a non-recursive depth first search to agglomerate adjacent voxels
          void depth_first_search (uint32_t root,
                                   cluster & cluster,
                                   std::vector<uint32_t> & labels,
                                   const std::vector<float> & data,
                                   const float threshold) const {
            uint32_t node = root;
            std::stack<uint32_t> stack;
            while (true) {
              labels[node] = cluster.label;
              stack.push (node);
              cluster.size++;
              if (next_neighbour (node, labels, data, threshold)) {
                continue;
              } else {
                do {
                  if (stack.top() == root)
                    return;
                  stack.pop();
                  node = stack.top();
                } while (!next_neighbour (node, labels, data, threshold));
              }
            }
          }


          std::vector<std::vector<int> > mask_indices_;
          std::vector<std::vector<uint32_t> > adjacent_indices_;
          Math::Matrix<float> dir_adjacency_matrix_;
          std::vector<bool> ignore_dim_;
          bool do_26_connectivity_;
      };




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
          ignore_dim_.resize(this->ndim(), false);
          largest_only_ = false;
          do_26_connectivity_ = false;
        }


        template <class InputVoxelType, class OutputVoxelType>
        void operator() (InputVoxelType& in, OutputVoxelType& out) {
          std::vector<cluster> clusters;
          std::vector<uint32_t> labels;

          Connector connector (do_26_connectivity_);
          if (directions_.is_set())
            connector.set_directions (directions_, angular_threshold_);
          connector.precompute_adjacency(in);
          for (size_t dim = 0; dim < in.ndim(); ++dim) 
            connector.set_ignore_dim (ignore_dim_[dim], dim);
          std::vector<std::vector<int> > mask_indices = connector.run (clusters, labels);
          std::sort (clusters.begin(), clusters.end(), compare_clusters);

          std::vector<int> label_lookup (clusters.size(), 0);
          for (uint32_t c = 0; c < clusters.size(); c++)
            label_lookup[clusters[c].label - 1] = c + 1;

          Image::LoopInOrder loop(out);
          for (loop.start(out); loop.ok(); loop.next(out))
            out.value() = 0;

          for (uint32_t i = 0; i < mask_indices.size(); i++) {
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


        void set_directions (Math::Matrix<float> & dirs_el_az, float angular_threshold) {
          directions_ = dirs_el_az;
          angular_threshold_ = angular_threshold;
        }


        void set_largest_only (bool largest_only) {
          largest_only_ = largest_only;
        }

        void set_26_connectivity (bool do_26_connectivity) {
          do_26_connectivity_ = do_26_connectivity;
        }


        protected:
          std::vector<int> ignore_dim_;
          bool largest_only_;
          Math::Matrix<float> directions_;
          float angular_threshold_;
          bool do_26_connectivity_;
      };
      //! @}
    }
  }
}


#endif
