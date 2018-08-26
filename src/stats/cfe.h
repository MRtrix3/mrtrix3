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


#ifndef __stats_cfe_h__
#define __stats_cfe_h__

#include <atomic>

#include "image.h"
#include "image_helpers.h"
#include "types.h"
#include "math/math.h"
#include "math/stats/typedefs.h"

#include "dwi/tractography/streamline.h"
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
      using connectivity_vector_type = Eigen::Array<connectivity_value_type, Eigen::Dynamic, 1>;
      using SetVoxelDir = DWI::Tractography::Mapping::SetVoxelDir;



      /** \addtogroup Statistics
      @{ */


      // TODO Classes for dealing with dynamic multi-threaded construction of the
      //   fixel-fixel connectivity matrix
      class InitMatrixElement
      { NOMEMALIGN
        public:
          //InitMatrixElement() = delete; // Can't delete this if calling vector.resize();
          InitMatrixElement() :
              fixel_index (std::numeric_limits<index_type>::max()),
              track_count (0) { }
          InitMatrixElement (const index_type fixel_index) :
              fixel_index (fixel_index),
              track_count (1) { }
          InitMatrixElement (const index_type fixel_index, const index_type track_count) :
              fixel_index (fixel_index),
              track_count (track_count) { }
          InitMatrixElement (const InitMatrixElement&) = default;
          FORCE_INLINE InitMatrixElement& operator++() { track_count++; return *this; }
          FORCE_INLINE InitMatrixElement& operator= (const InitMatrixElement& that) { fixel_index = that.fixel_index; track_count = that.track_count; return *this; }
          FORCE_INLINE index_type index() const { return fixel_index; }
          FORCE_INLINE index_type value() const { return track_count; }
          FORCE_INLINE bool operator< (const InitMatrixElement& that) const { return fixel_index < that.fixel_index; }
        private:
          index_type fixel_index;
          index_type track_count;
      };



      // TODO Ideally base class would be private, but need simple access to iterators for now
      class InitMatrixFixel : public vector<InitMatrixElement>
      { MEMALIGN(InitMatrixFixel)
        public:
          using BaseType = vector<InitMatrixElement>;
          InitMatrixFixel() :
              spinlock (ATOMIC_FLAG_INIT),
              track_count (0) { }
          void add (const vector<index_type>& indices);
        private:
          std::atomic_flag spinlock;
          index_type track_count;

          InitMatrixFixel& operator= (vector<InitMatrixElement>&& that) {
            this->BaseType::operator= (std::move (that));
            return *this;
          }
      };






      class connectivity { NOMEMALIGN
        public:
          connectivity () : value (0.0) { }
          connectivity (const connectivity_value_type v) : value (v) { }
          connectivity_value_type value;
      };


      // A class to store fixel index / connectivity value pairs
      //   only after the connectivity matrix has been thresholded / normalised
      class NormMatrixElement
      { NOMEMALIGN
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



      // With the internally normalised CFE expression, want to store a
      //   multiplicative factor per fixel
      class NormMatrixFixel : public vector<NormMatrixElement>
      { MEMALIGN(NormMatrixFixel)
        public:
          using BaseType = vector<NormMatrixElement>;
          NormMatrixFixel() :
            norm_multiplier (Stats::CFE::value_type (1.0)) { }
          NormMatrixFixel (const BaseType& i) :
              BaseType (i),
              norm_multiplier (Stats::CFE::value_type (1.0)) { }
          NormMatrixFixel (BaseType&& i) :
              BaseType (std::move (i)),
              norm_multiplier (Stats::CFE::value_type (1.0)) { }
          void normalise() {
            norm_multiplier = Stats::CFE::value_type (0.0);
            for (const auto& c : *this)
              norm_multiplier += c.value();
            norm_multiplier = Stats::CFE::value_type (1.0) / norm_multiplier;
          }
          Stats::CFE::value_type norm_multiplier;

      };




      // Different types are used depending on whether the connectivity matrix
      //   is in the process of being built, or whether it has been normalised
      //using init_connectivity_matrix_type = vector<std::map<index_type, connectivity>>;
      using init_connectivity_matrix_type = vector<InitMatrixFixel>;
      using norm_connectivity_matrix_type = vector<NormMatrixFixel>;



      /**
       * Process each track by converting each streamline to a set of dixels, and map these to fixels.
       */
      // TODO Modify this by incorporating the track mapping functor, doing the dixel->fixel mapping,
      //   then calling the appropriate add() functions within the initial connectivity matrix
      // TODO Eventually check whether or not it would be preferable to remove the explicit fixel TDI
      class TrackProcessor { MEMALIGN(TrackProcessor)

        public:
          TrackProcessor (const DWI::Tractography::Mapping::TrackMapperBase& mapper,
                          Image<index_type>& fixel_indexer,
                          const vector<direction_type>& fixel_directions,
                          Image<bool>& fixel_mask,
                          vector<uint16_t>& fixel_TDI,
                          init_connectivity_matrix_type& connectivity_matrix,
                          const value_type angular_threshold);

          //bool operator () (const SetVoxelDir& in);
          bool operator () (const DWI::Tractography::Streamline<>& in);

        private:
          const DWI::Tractography::Mapping::TrackMapperBase& mapper;
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
          virtual ~Enhancer() { }

        protected:
          const norm_connectivity_matrix_type& connectivity_matrix;
          const value_type dh, E, H;

          void operator() (in_column_type, out_column_type) const override;
      };


      //! @}

    }
  }
}

#endif
