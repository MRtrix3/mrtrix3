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

#ifndef __stats_cluster_h__
#define __stats_cluster_h__

#include "filter/connected_components.h"
#include "math/stats/typedefs.h"

#include "stats/tfce.h"
#include "stats/enhance.h"

namespace MR
{
  namespace Stats
  {
    namespace Cluster
    {


      using value_type = Math::Stats::value_type;
      using vector_type = Math::Stats::vector_type;


      /** \addtogroup Statistics
      @{ */
      class ClusterSize : public Stats::TFCE::EnhancerBase
      { MEMALIGN (ClusterSize)
        public:
          ClusterSize (const Filter::Connector& connector, const value_type T) :
                       connector (connector), threshold (T) { }
          virtual ~ClusterSize() { }

          void set_threshold (const value_type T) { threshold = T; }

        protected:
          const Filter::Connector& connector;
          value_type threshold;

          void operator() (in_column_type in, out_column_type out) const override {
            (*this) (in, threshold, out);
          }

          void operator() (in_column_type, const value_type, out_column_type) const override;
      };
      //! @}



    }
  }
}

#endif
