/* Copyright (c) 2008-2023 the MRtrix3 contributors.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Covered Software is provided under this License on an "as is"
 * basis, without warranty of any kind, either expressed, implied, or
 * statutory, including, without limitation, warranties that the
 * Covered Software is free of defects, merchantable, fit for a
 * particular purpose or non-infringing.
 * See the Mozilla Public License v. 2.0 for more details.
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
