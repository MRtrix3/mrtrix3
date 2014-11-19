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
#ifndef __stats_tfce_h__
#define __stats_tfce_h__

#include <gsl/gsl_linalg.h>

#include "math/vector.h"
#include "math/matrix.h"
#include "math/stats/permutation.h"
#include "image/filter/connected_components.h"
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

      class Enhancer {
        public:
          Enhancer (const Image::Filter::Connector& connector, const value_type dh, const value_type E, const value_type H) :
                    connector (connector), dh (dh), E (E), H (H) {}

          value_type operator() (const value_type max_stat, const std::vector<value_type>& stats,
                                 std::vector<value_type>& enhanced_stats) const
          {
            enhanced_stats.resize(stats.size());
            std::fill (enhanced_stats.begin(), enhanced_stats.end(), 0.0);

            for (value_type h = this->dh; h < max_stat; h += this->dh) {
              std::vector<Image::Filter::cluster> clusters;
              std::vector<uint32_t> labels (enhanced_stats.size(), 0);
              connector.run (clusters, labels, stats, h);
              for (size_t i = 0; i < enhanced_stats.size(); ++i)
                if (labels[i])
                  enhanced_stats[i] += pow (clusters[labels[i]-1].size, this->E) * pow (h, this->H);
            }

            return *std::max_element (enhanced_stats.begin(), enhanced_stats.end());
          }

        protected:
          const Image::Filter::Connector& connector;
          const value_type dh, E, H;
      };

      //! @}
    }
  }
}

#endif
