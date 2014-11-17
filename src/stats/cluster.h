/*
    Copyright 2011 Brain Research Institute, Melbourne, Australia

    Written by David Raffelt and Donald Tournier 23/07/11.

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
#ifndef __stats_cluster_h__
#define __stats_cluster_h__


#include "image/filter/connected_components.h"

namespace MR
{
  namespace Stats
  {
    namespace Cluster
    {

      typedef float value_type;


      /** \addtogroup Statistics
      @{ */
      class ClusterSize {
        public:
          ClusterSize (const Image::Filter::Connector& connector, value_type cluster_forming_threshold) :
                      connector (connector), cluster_forming_threshold (cluster_forming_threshold) { }

          value_type operator() (const value_type unused, const std::vector<value_type>& stats,
                                 std::vector<value_type>& get_cluster_sizes) const
          {
            std::vector<Image::Filter::cluster> clusters;
            std::vector<uint32_t> labels (stats.size(), 0);
            connector.run (clusters, labels, stats, cluster_forming_threshold);
            get_cluster_sizes.resize (stats.size());
            for (size_t i = 0; i < stats.size(); ++i)
              get_cluster_sizes[i] = labels[i] ? clusters[labels[i]-1].size : 0.0;

            return clusters.size() ? std::max_element (clusters.begin(), clusters.end())->size : 0.0;
          }

        protected:
          const Image::Filter::Connector& connector;
          value_type cluster_forming_threshold;
      };

      //! @}

    }
  }
}

#endif
