/* Copyright (c) 2008-2017 the MRtrix3 contributors
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


#ifndef __stats_cfe_h__
#define __stats_cfe_h__

#include "image.h"
#include "image_helpers.h"
#include "math/math.h"
#include "math/stats/typedefs.h"

#include "dwi/tractography/mapping/mapper.h"
#include "stats/enhance.h"

namespace MR
{
  namespace Stats
  {
    namespace CFE
    {

      typedef Math::Stats::value_type value_type;
      typedef Math::Stats::vector_type vector_type;
      typedef float connectivity_value_type;
      typedef Eigen::Matrix<value_type, 3, 1> direction_type;
      typedef Eigen::Array<connectivity_value_type, Eigen::Dynamic, 1> connectivity_vector_type;
      typedef DWI::Tractography::Mapping::SetVoxelDir SetVoxelDir;


      /** \addtogroup Statistics
      @{ */


      class connectivity { MEMALIGN(connectivity)
        public:
          connectivity () : value (0.0) { }
          connectivity (const connectivity_value_type v) : value (v) { }
          connectivity_value_type value;
      };




      /**
       * Process each track by converting each streamline to a set of dixels, and map these to fixels.
       */
      class TrackProcessor { MEMALIGN(TrackProcessor)

        public:
          TrackProcessor (Image<uint32_t>& fixel_indexer,
                          const vector<direction_type>& fixel_directions,
                          vector<uint16_t>& fixel_TDI,
                          vector<std::map<uint32_t, connectivity> >& connectivity_matrix,
                          const value_type angular_threshold);

          bool operator () (const SetVoxelDir& in);

        private:
          Image<uint32_t> fixel_indexer;
          const vector<direction_type>& fixel_directions;
          vector<uint16_t>& fixel_TDI;
          vector<std::map<uint32_t, connectivity> >& connectivity_matrix;
          const value_type angular_threshold_dp;
      };




      class Enhancer : public Stats::EnhancerBase { MEMALIGN (Enhancer)
        public:
          Enhancer (const vector<std::map<uint32_t, connectivity> >& connectivity_map,
                    const value_type dh, const value_type E, const value_type H);


          value_type operator() (const vector_type& stats, vector_type& enhanced_stats) const override;


        protected:
          const vector<std::map<uint32_t, connectivity> >& connectivity_map;
          const value_type dh, E, H;
      };


      //! @}

    }
  }
}

#endif
