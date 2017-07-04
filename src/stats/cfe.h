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


#ifndef __stats_cfe_h__
#define __stats_cfe_h__

#include "image.h"
#include "image_helpers.h"
#include "types.h"
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

      using index_type = uint32_t;
      using value_type = Math::Stats::value_type;
      using vector_type = Math::Stats::vector_type;
      using connectivity_value_type = float;
      using direction_type = Eigen::Matrix<value_type, 3, 1>;
      using SetVoxelDir = DWI::Tractography::Mapping::SetVoxelDir;



      /** \addtogroup Statistics
      @{ */


      class connectivity { NOMEMALIGN
        public:
          connectivity () : value (0.0) { }
          connectivity (const connectivity_value_type v) : value (v) { }
          connectivity_value_type value;
      };


      // A class to store fixel index / connectivity value pairs
      //   only after the connectivity matrix has been thresholded / normalised
      class NormMatrixElement
      {
        public:
          NormMatrixElement (const index_type fixel_index,
                             const connectivity_value_type connectivity_value) :
              fixel_index (fixel_index),
              connectivity_value (connectivity_value) { }
          FORCE_INLINE index_type index() const { return fixel_index; }
          FORCE_INLINE connectivity_value_type value() const { return connectivity_value; }
          FORCE_INLINE void normalise (const connectivity_value_type norm_factor) { connectivity_value *= norm_factor; }
        private:
          const index_type fixel_index;
          connectivity_value_type connectivity_value;
      };



      // Different types are used depending on whether the connectivity matrix
      //   is in the process of being built, or whether it has been normalised
      using init_connectivity_matrix_type = vector<std::map<index_type, connectivity>>;
      using norm_connectivity_matrix_type = vector<vector<NormMatrixElement>>;



      /**
       * Process each track by converting each streamline to a set of dixels, and map these to fixels.
       */
      class TrackProcessor { MEMALIGN(TrackProcessor)

        public:
          TrackProcessor (Image<index_type>& fixel_indexer,
                          const vector<direction_type>& fixel_directions,
                          Image<bool>& fixel_mask,
                          vector<uint16_t>& fixel_TDI,
                          init_connectivity_matrix_type& connectivity_matrix,
                          const value_type angular_threshold);

          bool operator () (const SetVoxelDir& in);

        private:
          Image<index_type> fixel_indexer;
          const vector<direction_type>& fixel_directions;
          Image<bool> fixel_mask;
          vector<uint16_t>& fixel_TDI;
          init_connectivity_matrix_type& connectivity_matrix;
          const value_type angular_threshold_dp;
      };




      class Enhancer : public Stats::EnhancerBase { MEMALIGN (Enhancer)
        public:
          Enhancer (const norm_connectivity_matrix_type& connectivity_matrix,
                    const value_type dh, const value_type E, const value_type H);


          value_type operator() (const vector_type& stats, vector_type& enhanced_stats) const override;


        protected:
          const norm_connectivity_matrix_type& connectivity_matrix;
          const value_type dh, E, H;
      };


      //! @}

    }
  }
}

#endif
