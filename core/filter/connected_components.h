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


#ifndef __filter_connected_h__
#define __filter_connected_h__

#include "memory.h"
#include "image.h"
#include "algo/loop.h"

#include "filter/base.h"

#include <stack>
#include <iostream>

namespace MR
{
  namespace Filter
  {

    class cluster { NOMEMALIGN
      public:
        uint32_t label;
        uint32_t size;
        bool operator< (const cluster& j) const {
          return size < j.size;
        }
    };


    inline bool compare_clusters (const cluster& i, const cluster& j)
    {
      return (i.size > j.size);
    }


    class Connector { NOMEMALIGN

      public:
        Connector (bool do_26_connectivity) :
          do_26_connectivity (do_26_connectivity),
          dim_to_ignore (4, false) {
            dim_to_ignore[3] = true;
        }


        // Perform connected components on the mask.
        const vector<vector<int> >& run (vector<cluster>& clusters,
                                                   vector<uint32_t>& labels) const {
          labels.resize (adjacent_indices.size(), 0);
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
          return mask_indices;
        }


        // Perform connected components on data with the defined threshold. Assumes adjacency is the same as the mask.
        template <class VectorType>
        void run (vector<cluster>& clusters,
                  vector<uint32_t>& labels,
                  const VectorType& data,
                  const float threshold) const {
          labels.resize (adjacent_indices.size(), 0);
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


        void set_dim_to_ignore (vector<bool>& ignore_dim) {
          for (size_t d = 0; d < ignore_dim.size(); ++d) {
            dim_to_ignore[d] = ignore_dim[d];
          }
        }


        template <class MaskImageType>
        const vector<vector<int> >& precompute_adjacency (MaskImageType& mask) {

          auto index_image = Image<uint32_t>::scratch (mask);

          // 1st pass, store mask image indices and their index in the array
          for (auto l = Loop (mask) (mask, index_image); l; ++l) {
            if (mask.value() >= 0.5) {
              // For each voxel, store the index within mask_indices for 2nd pass
              index_image.value() = mask_indices.size();
              vector<int> index (mask.ndim());
              for (size_t dim = 0; dim < mask.ndim(); dim++)
                index[dim] = mask.index(dim);
              mask_indices.push_back (index);
            } else {
              index_image.value() = 0;
            }
          }
          // Here we pre-compute the offsets for our neighbours in 4D space
          vector< vector<int> > neighbour_offsets;
          vector<int> offset (4);
          for (offset[0] = -1; offset[0] <= 1; offset[0]++) {
            for (offset[1] = -1; offset[1] <= 1; offset[1]++) {
              for (offset[2] = -1; offset[2] <= 1; offset[2]++) {
                for (offset[3] = -1; offset[3] <= 1; offset[3]++) {
                  if (!do_26_connectivity && ((abs(offset[0]) + abs(offset[1]) + abs(offset[2]) + abs(offset[3])) > 1))
                    continue;
                  if ((abs(offset[0]) && dim_to_ignore[0]) || (abs(offset[1]) && dim_to_ignore[1]) ||
                      (abs(offset[2]) && dim_to_ignore[2]) || (abs(offset[3]) && dim_to_ignore[3]))
                    continue;
                  neighbour_offsets.push_back (offset);
                }
              }
            }
          }
          // 2nd pass, define adjacency
          MaskImageType mask_neigh (mask);
          for (vector<vector<int> >::const_iterator it = mask_indices.begin(); it != mask_indices.end(); ++it) {
            vector<uint32_t> neighbour_indices;
            for (vector< vector<int> >::const_iterator offset = neighbour_offsets.begin(); offset != neighbour_offsets.end(); ++offset) {
              for (size_t dim = 0; dim < mask.ndim(); dim++)
                mask_neigh.index(dim) = (*it)[dim] + (*offset)[dim];
              if (!is_out_of_bounds (mask_neigh)) {
                if (mask_neigh.value() >= 0.5) {
                  assign_pos_of (mask_neigh).to (index_image);
                  neighbour_indices.push_back (index_image.value());
                }
              }
            }
            adjacent_indices.push_back (neighbour_indices);
          }

          return mask_indices;
        }


        bool next_neighbour (uint32_t& node, vector<uint32_t>& labels) const {
          for (size_t n = 0; n < adjacent_indices[node].size(); n++) {
            if (labels[adjacent_indices[node][n]] == 0) {
              node = adjacent_indices[node][n];
              return true;
            }
          }
          return false;
        }


        template <class VectorType>
        bool next_neighbour (uint32_t& node,
                             vector<uint32_t>& labels,
                             const VectorType& data,
                             const float threshold) const {
          for (size_t n = 0; n < adjacent_indices[node].size(); n++) {
            if (labels[adjacent_indices[node][n]] == 0 && data[adjacent_indices[node][n]] > threshold) {
              node = adjacent_indices[node][n];
              return true;
            }
          }
          return false;
        }


        // use a non-recursive depth first search to agglomerate adjacent voxels
        void depth_first_search (uint32_t root,
                                 cluster& cluster,
                                 vector<uint32_t>& labels) const {
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
        template <class VectorType>
        void depth_first_search (uint32_t root,
                                 cluster& cluster,
                                 vector<uint32_t>& labels,
                                 const VectorType& data,
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


        bool do_26_connectivity;
        vector<bool> dim_to_ignore;
        vector<vector<int> > mask_indices;
        vector<vector<uint32_t> > adjacent_indices;
    };




    /** \addtogroup Filters
    @{ */

    /*! Label all connected components within a binary mask of n-dimensions.
     *  This filter will label each component in order of increasing component size
     *
     * Typical usage:
     * \code
     * auto input = Image<bool>::open (argument[0]);
     * Filter::ConnectedComponents filter (input);
     * auto output = Image<uint32_t>::create (argument[1], filter);
     * filter (input, output);
     *
     * \endcode
     */
    class ConnectedComponents : public Base { MEMALIGN(ConnectedComponents)
      public:

      template <class HeaderType>
      ConnectedComponents (const HeaderType& in) :
        Base (in),
        largest_only (false),
        do_26_connectivity (false)
      {
        if (this->ndim() > 4)
          throw Exception ("Cannot run connected components analysis with more than 4 dimensions");
        datatype_ = DataType::UInt32;
        dim_to_ignore.resize (this->ndim(), false);
        if (this->ndim() == 4) // Ignore 4D unless explicitly instructed to
          dim_to_ignore[3] = true;
      }

      template <class HeaderType>
      ConnectedComponents (const HeaderType& in, const std::string& message) :
        ConnectedComponents (in)
      {
        set_message (message);
      }


      template <class InputVoxelType, class OutputVoxelType>
      void operator() (InputVoxelType& in, OutputVoxelType& out)
      {
        Connector connector (do_26_connectivity);

        if (dim_to_ignore.size())
          connector.set_dim_to_ignore (dim_to_ignore);

        connector.precompute_adjacency (in);

        std::unique_ptr<ProgressBar> progress;
        if (message.size()) {
          progress.reset (new ProgressBar (message));
          ++(*progress);
        }

        vector<cluster> clusters;
        vector<uint32_t> labels;
        vector<vector<int> > mask_indices = connector.run (clusters, labels);

        if (progress)
          ++(*progress);
        std::sort (clusters.begin(), clusters.end(), compare_clusters);
        if (progress)
          ++(*progress);

        vector<int> label_lookup (clusters.size(), 0);
        for (uint32_t c = 0; c < clusters.size(); c++)
          label_lookup[clusters[c].label - 1] = c + 1;

        for (auto l = Loop (out) (out); l; ++l)
          out.value() = 0;

        for (uint32_t i = 0; i < mask_indices.size(); i++)
        {
          assign_pos_of (mask_indices[i]).to (out);
          if (largest_only) {
            if (label_lookup[labels[i] - 1] == 1)
              out.value() = 1;
          } else {
            out.value() = label_lookup[labels[i] - 1];
          }
        }
      }


      void set_ignore_dim (size_t dim, bool ignore)
      {
        assert (dim < this->ndim());
        dim_to_ignore[dim] = ignore;
      }


      void set_largest_only (bool value)
      {
        largest_only = value;
      }


      void set_26_connectivity (bool value)
      {
        do_26_connectivity = value;
      }


      protected:
        vector<bool> dim_to_ignore;
        bool largest_only;
        bool do_26_connectivity;
    };
    //! @}
  }
}


#endif
