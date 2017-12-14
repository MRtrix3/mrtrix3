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


#include "stats/cluster.h"

namespace MR
{
  namespace Stats
  {
    namespace Cluster
    {



      void ClusterSize::operator() (in_column_type input, const value_type T, out_column_type output) const
      {
        vector<Filter::Connector::Cluster> clusters;
        vector<uint32_t> labels (input.size(), 0);
        connector.run (clusters, labels, input, T);
        output.resize (input.size());
        for (size_t i = 0; i < size_t(input.size()); ++i)
          output[i] = labels[i] ? clusters[labels[i]-1].size : 0.0;
      }



    }
  }
}
