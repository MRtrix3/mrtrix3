/*
 * Copyright (c) 2008-2016 the MRtrix3 contributors
 * 
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/
 * 
 * MRtrix is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * 
 * For more details, see www.mrtrix.org
 * 
 */

#include "stats/cluster.h"

#include <algorithm>

namespace MR
{
  namespace Stats
  {
    namespace Cluster
    {



      value_type ClusterSize::operator() (const vector_type& stats, const value_type T, vector_type& get_cluster_sizes) const
      {
        std::vector<Filter::cluster> clusters;
        std::vector<uint32_t> labels (stats.size(), 0);
        connector.run (clusters, labels, stats, T);
        get_cluster_sizes.resize (stats.size());
        for (size_t i = 0; i < size_t(stats.size()); ++i)
          get_cluster_sizes[i] = labels[i] ? clusters[labels[i]-1].size : 0.0;

        return clusters.size() ? std::max_element (clusters.begin(), clusters.end())->size : 0.0;
      }



    }
  }
}
