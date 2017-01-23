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
#ifndef __stats_cluster_h__
#define __stats_cluster_h__


#include "filter/connected_components.h"

namespace MR
{
  namespace Stats
  {
    namespace Cluster
    {

      typedef float value_type;


      /** \addtogroup Statistics
      @{ */
      class ClusterSize { MEMALIGN(ClusterSize)
        public:
          ClusterSize (const Filter::Connector& connector, value_type cluster_forming_threshold) :
                       connector (connector), cluster_forming_threshold (cluster_forming_threshold) { }

          value_type operator() (const value_type unused, const vector<value_type>& stats,
                                 vector<value_type>& get_cluster_sizes) const
          {
            vector<Filter::cluster> clusters;
            vector<uint32_t> labels (stats.size(), 0);
            connector.run (clusters, labels, stats, cluster_forming_threshold);
            get_cluster_sizes.resize (stats.size());
            for (size_t i = 0; i < stats.size(); ++i)
              get_cluster_sizes[i] = labels[i] ? clusters[labels[i]-1].size : 0.0;

            return clusters.size() ? std::max_element (clusters.begin(), clusters.end())->size : 0.0;
          }

        protected:
          const Filter::Connector& connector;
          value_type cluster_forming_threshold;
      };

      //! @}

    }
  }
}

#endif
