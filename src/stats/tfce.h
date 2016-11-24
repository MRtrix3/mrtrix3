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
#ifndef __stats_tfce_h__
#define __stats_tfce_h__

#include "math/stats/permutation.h"
#include "filter/connected_components.h"
#include "thread_queue.h"

namespace MR
{
  namespace Stats
  {
    namespace TFCE
    {

      typedef float value_type;


      /** \addtogroup Statistics
      @{ */

      class Enhancer { MEMALIGN(Enhancer)
        public:
          Enhancer (const Filter::Connector& connector, const value_type dh, const value_type E, const value_type H) :
                    connector (connector), dh (dh), E (E), H (H) {}

          value_type operator() (const value_type max_stat, const std::vector<value_type>& stats,
                                 std::vector<value_type>& enhanced_stats) const
          {
            enhanced_stats.resize(stats.size());
            std::fill (enhanced_stats.begin(), enhanced_stats.end(), 0.0);

            for (value_type h = this->dh; h < max_stat; h += this->dh) {
              std::vector<Filter::cluster> clusters;
              std::vector<uint32_t> labels (enhanced_stats.size(), 0);
              connector.run (clusters, labels, stats, h);
              for (size_t i = 0; i < enhanced_stats.size(); ++i)
                if (labels[i])
                  enhanced_stats[i] += pow (clusters[labels[i]-1].size, this->E) * pow (h, this->H);
            }

            return *std::max_element (enhanced_stats.begin(), enhanced_stats.end());
          }

        protected:
          const Filter::Connector& connector;
          const value_type dh, E, H;
      };

      //! @}
    }
  }
}

#endif
