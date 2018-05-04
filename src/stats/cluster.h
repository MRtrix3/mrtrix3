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
      class ClusterSize : public Stats::TFCE::EnhancerBase { MEMALIGN (ClusterSize)
        public:
          ClusterSize (const Filter::Connector& connector, const value_type T) :
                       connector (connector), threshold (T) { }
          virtual ~ClusterSize() { }

          void set_threshold (const value_type T) { threshold = T; }


          value_type operator() (const vector_type& in, vector_type& out) const override {
            return (*this) (in, threshold, out);
          }

          value_type operator() (const vector_type&, const value_type, vector_type&) const override;


        protected:
          const Filter::Connector& connector;
          value_type threshold;
      };
      //! @}



    }
  }
}

#endif
