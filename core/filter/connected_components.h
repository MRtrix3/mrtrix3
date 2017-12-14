/* Copyright (c) 2008-2017 the MRtrix3 contributors.
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


#ifndef __filter_connected_h__
#define __filter_connected_h__


#include "image.h"
#include "memory.h"
#include "types.h"

#include "filter/base.h"
#include "misc/voxel2vector.h"

#include <stack>
#include <iostream>


namespace MR
{
  namespace Filter
  {



    class Connector
    { NOMEMALIGN

      public:

        // A class that pre-computes and stores, for each voxel, a
        //   list of voxels (represented as indices) that are adjacent
        //
        // If we were to re-implement dixel-wise connectivity, it would
        //   be done using an alternative initialise() function for this
        //   class, to define the volumes on the fourth axis that
        //   correspond to neighbouring directions using a Directions::Set.
        class Adjacency
        {
          public:
            typedef Voxel2Vector::index_t index_t;

            Adjacency() :
                use_26_neighbours (false),
                enabled_axes (3, true) { }

            void toggle_axis (const size_t axis, const bool value) {
              if (axis > enabled_axes.size())
                enabled_axes.resize (axis+1, false);
              enabled_axes[axis] = value;
              data.clear();
            }

            void set_axes (const vector<bool>& i) {
              enabled_axes = i;
              data.clear();
            }

            void initialise (const Header&, const Voxel2Vector&);

            const vector<index_t>& operator[] (const size_t index) const {
              assert (size());
              assert (index < size());
              return data[index];
            }

            void set_26_adjacency (const bool i) {
              use_26_neighbours = i;
              data.clear();
            }

            size_t size() const { return data.size(); }

          private:
            bool use_26_neighbours;
            vector<bool> enabled_axes;
            vector<vector<index_t>> data;
        } adjacency;



        class Cluster
        { NOMEMALIGN
          public:
            Cluster (const uint32_t l) :
                label (l),
                size (0) { }
            uint32_t label;
            uint32_t size;
            bool operator< (const Cluster& j) const {
              return size < j.size;
            }
        };
        // Used for sorting clusters in order of size
        static bool largest (const Cluster& i, const Cluster& j) {
          return (i.size > j.size);
        }




        Connector () { }

        // Perform connected components on vectorized binary data
        void run (vector<Cluster>&, vector<uint32_t>&) const;
        template <class VectorType>
        void run (vector<Cluster>&, vector<uint32_t>&,
                  const VectorType&, const float) const;


      private:

        // Utility functions that perform the actual connected
        //   components functionality
        bool next_neighbour (uint32_t&, vector<uint32_t>&) const;
        template <class VectorType>
        bool next_neighbour (uint32_t&, vector<uint32_t>&,
                             const VectorType&, const float) const;

        void depth_first_search (const uint32_t, Cluster&, vector<uint32_t>&) const;
        template <class VectorType>
        void depth_first_search (const uint32_t, Cluster&, vector<uint32_t>&,
                                 const VectorType&, const float) const;


    };



    template <class VectorType>
    void Connector::run (vector<Cluster>& clusters,
                         vector<uint32_t>& labels,
                         const VectorType& data,
                         const float threshold) const
    {
      assert (adjacency.size());
      labels.resize (adjacency.size(), 0);
      uint32_t current_label = 0;
      for (uint32_t i = 0; i < labels.size(); i++) {
        // This node has not been already clustered and is above threshold
        if (!labels[i] && data[i] > threshold) {
          Cluster cluster (++current_label);
          depth_first_search (i, cluster, labels, data, threshold);
          clusters.push_back (cluster);
        }
      }
      if (clusters.size() > std::numeric_limits<uint32_t>::max())
        throw Exception ("The number of clusters is larger than can be labelled with an unsigned 32bit integer.");
    }



    template <class VectorType>
    bool Connector::next_neighbour (uint32_t& node,
                                    vector<uint32_t>& labels,
                                    const VectorType& data,
                                    const float threshold) const
    {
      for (auto n : adjacency[node]) {
        if (!labels[n] && data[n] > threshold) {
          node = n;
          return true;
        }
      }
      return false;
    }



    template <class VectorType>
    void Connector::depth_first_search (const uint32_t root,
                                        Cluster& cluster,
                                        vector<uint32_t>& labels,
                                        const VectorType& data,
                                        const float threshold) const
    {
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
            enabled_axes (ndim(), true),
            largest_only (false),
            do_26_connectivity (false)
        {
          if (this->ndim() > 4)
            throw Exception ("Cannot run connected components analysis with more than 4 dimensions");
          datatype_ = DataType::UInt32;
          // By default, ignore all axes above the three spatial dimensions
          for (size_t axis = 3; axis < ndim(); ++axis)
            enabled_axes[axis] = false;
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
          Voxel2Vector v2v (in, *this);

          Connector connector;
          connector.adjacency.set_axes (enabled_axes);
          connector.adjacency.set_26_adjacency (do_26_connectivity);
          connector.adjacency.initialise (in, v2v);

          std::unique_ptr<ProgressBar> progress;
          if (message.size()) {
            progress.reset (new ProgressBar (message));
            ++(*progress);
          }

          vector<Connector::Cluster> clusters;
          vector<uint32_t> labels;
          connector.run (clusters, labels);
          if (progress) ++(*progress);

          // Sort clusters in order from largest to smallest
          std::sort (clusters.begin(), clusters.end(), Connector::largest);
          if (progress) ++(*progress);

          // Generate a lookup table to map input cluster index to
          //   output cluster index following cluster-size sorting
          vector<uint32_t> index_lookup (clusters.size() + 1, 0);
          for (uint32_t c = 0; c < clusters.size(); c++)
            index_lookup[clusters[c].label] = c + 1;

          for (auto l = Loop (out) (out); l; ++l)
            out.value() = 0;

          for (uint32_t i = 0; i < v2v.size(); i++) {
            assign_pos_of (v2v[i]).to (out);
            if (largest_only) {
              if (index_lookup[labels[i]] == 1)
                out.value() = 1;
            } else {
              out.value() = index_lookup[labels[i]];
            }
          }
        }



        void set_axes (const vector<int>& i)
        {
          const size_t max_axis = *std::max_element (i.begin(), i.end());
          if (max_axis >= ndim())
            throw Exception ("Requested axis for connected component filter (" + str(max_axis) + " is beyond the dimensionality of the image (" + str(ndim()) + "D)");
          enabled_axes.assign (std::max (max_axis+1, size_t(ndim())), false);
          for (const auto& axis : i) {
            if (axis < 0)
              throw Exception ("Cannot specify negative axis index for connected-component filter");
            enabled_axes[axis] = true;
          }
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
        vector<bool> enabled_axes;
        bool largest_only;
        bool do_26_connectivity;
    };
    //! @}
  }
}


#endif
