/* Copyright (c) 2008-2017 the MRtrix3 contributors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, you can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * MRtrix is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * For more details, see http://www.mrtrix.org/.
 */


#include "filter/connected_components.h"

namespace MR
{
  namespace Filter
  {



    void Connector::Adjacency::initialise (const Header& header, const Voxel2Vector& v2v)
    {
      data.clear();
      // Simplify handling of 4D images: don't need to keep checking
      //   size of axes against number of image dimensions
      if (header.ndim() < 3)
        throw Exception ("Connected components filter not designed to handle less than 3 axes");
      if (header.ndim() > enabled_axes.size())
        enabled_axes.resize (header.ndim(), false);
      // Begin by disabling adjacency offsets for those axes for which adjacency is not permitted
      vector< vector<int> > offsets;
      vector<int> o (header.ndim(), -1);
      size_t start_axis = 0;
      for (size_t axis = 0; axis != header.ndim(); ++axis) {
        if (!enabled_axes[axis]) {
          o[axis] = 0;
          if (start_axis == axis)
            ++start_axis;
        }
      }
      if (start_axis == header.ndim())
        throw Exception ("Cannot initialise connected component filter: All axes have been disabled");
      // Now generate a list of plausible offsets between adjacent elements
      while (*std::max_element (o.begin(), o.end()) < 2) {
        // Determine whether or not this offset should be added to the list
        if (!use_26_neighbours && header.ndim() >= 3 && ((std::abs(o[0]) + std::abs(o[1]) + std::abs(o[2])) > 1))
          continue;
        offsets.push_back (o);
        // Find the next offset to be tested
        ++o[start_axis];
        for (size_t axis = start_axis; axis != header.ndim(); ++axis) {
          if (enabled_axes[axis]) {
            if (o[axis] == 2 && axis < header.ndim()-1) {
              o[axis] = -1;
              ++o[axis+1];
            }
          }
        }
      }
      // Now we can generate, for each element in the image, a list of adjacent elements
      // This may appear different to previous code, given the use of the Voxel2Vector class
      vector<index_t> pos (header.ndim());
      vector<int> neighbour (header.ndim());
      data.reserve (v2v.size());
      for (size_t i = 0; i != v2v.size(); ++i) {
        pos = v2v[i];
        vector<index_t> indices;
        for (auto o : offsets) {
          for (size_t axis = 0; axis != header.ndim(); ++axis)
            neighbour[axis] = pos[axis] + o[axis];
          // Is this a valid neighbour position, i.e. within the mask?
          // If so, the Voxel2vector class should provide us with a valid
          //   index of this neighbouring element
          const index_t j = v2v (neighbour);
          if (j != v2v.invalid)
            indices.push_back (j);
        }
        data.push_back (indices);
      }
      DEBUG("Adjacency data for " + str(data.size()) + " voxels initialised");
    }



    void Connector::run (vector<Cluster>& clusters,
                          vector<uint32_t>& labels) const
    {
      assert (adjacency.size());
      labels.resize (adjacency.size(), 0);
      uint32_t current_label = 0;
      for (uint32_t i = 0; i < labels.size(); i++) {
        // This node has not been already clustered
        if (!labels[i]) {
          Cluster cluster (++current_label);
          depth_first_search (i, cluster, labels);
          clusters.push_back (cluster);
        }
      }
      if (clusters.size() > std::numeric_limits<uint32_t>::max())
        throw Exception ("The number of clusters is larger than can be labelled with an unsigned 32bit integer.");
    }



    bool Connector::next_neighbour (uint32_t& node, vector<uint32_t>& labels) const
    {
      for (auto n : adjacency[node]) {
        if (!labels[n]) {
          node = n;
          return true;
        }
      }
      return false;
    }



    void Connector::depth_first_search (const uint32_t root,
                                         Cluster& cluster,
                                         vector<uint32_t>& labels) const
    {
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



  }
}
