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

#include "image.h"
#include "image_helpers.h"
#include "types.h"
#include "math/math.h"
#include "math/stats/typedefs.h"

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



      /** \addtogroup Statistics
      @{ */


      // Classes for dealing with dynamic multi-threaded construction of the
      //   fixel-fixel connectivity matrix
      class InitMatrixElement
      { NOMEMALIGN
        public:
          using ValueType = index_type;
          InitMatrixElement() :
              fixel_index (std::numeric_limits<index_type>::max()),
              track_count (0) { }
          InitMatrixElement (const index_type fixel_index) :
              fixel_index (fixel_index),
              track_count (1) { }
          InitMatrixElement (const index_type fixel_index, const ValueType track_count) :
              fixel_index (fixel_index),
              track_count (track_count) { }
          InitMatrixElement (const InitMatrixElement&) = default;
          FORCE_INLINE InitMatrixElement& operator++() { track_count++; return *this; }
          FORCE_INLINE InitMatrixElement& operator= (const InitMatrixElement& that) { fixel_index = that.fixel_index; track_count = that.track_count; return *this; }
          FORCE_INLINE index_type index() const { return fixel_index; }
          FORCE_INLINE ValueType value() const { return track_count; }
          FORCE_INLINE bool operator< (const InitMatrixElement& that) const { return fixel_index < that.fixel_index; }
        private:
          index_type fixel_index;
          ValueType track_count;
      };



      class InitMatrixFixel : public vector<InitMatrixElement>
      { MEMALIGN(InitMatrixFixel)
        public:
          using BaseType = vector<InitMatrixElement>;
          InitMatrixFixel() :
              track_count (0) { }
          void add (const vector<index_type>& indices);
          index_type count() const { return track_count; }
        private:
          index_type track_count;
      };






      // A class to store fixel index / connectivity value pairs
      //   only after the connectivity matrix has been thresholded / normalised
      class NormMatrixElement
      { NOMEMALIGN
        public:
          using ValueType = connectivity_value_type;
          NormMatrixElement (const index_type fixel_index,
                             const ValueType connectivity_value) :
              fixel_index (fixel_index),
              connectivity_value (connectivity_value) { }
          FORCE_INLINE index_type index() const { return fixel_index; }
          FORCE_INLINE ValueType value() const { return connectivity_value; }
          FORCE_INLINE void exponentiate (const ValueType C) { connectivity_value = std::pow (connectivity_value, C); }
          FORCE_INLINE void normalise (const ValueType norm_factor) { connectivity_value *= norm_factor; }
        private:
          index_type fixel_index;
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
      using init_connectivity_matrix_type = vector<InitMatrixFixel>;
      using norm_connectivity_matrix_type = vector<NormMatrixFixel>;



      // TODO Consider wrapping the fixel2column functionality into a class
      // In particular it should be made easier to define the "default" mapping
      //   in order to run the normalise_matrix() function (though it would
      //   also be used in fixelcfestats if no mask is provided)
      // Should probably have the reverse mapping functionality wrapped as well
      // It could also store the number of columns for easy access
      class FixelIndexMapper
      { NOMEMALIGN
        public:
          FixelIndexMapper () { }
          FixelIndexMapper (const index_type num_fixels);
          FixelIndexMapper (Image<bool> fixel_mask);

          index_type e2i (const index_type e) const {
            assert (e < num_external());
            return external2internal[e];
          }

          index_type i2e (const index_type i) const {
            assert (i < num_internal());
            return internal2external[i];
          }

          index_type num_external() const { return external2internal.size(); }
          index_type num_internal() const { return internal2external.size(); }

          static constexpr index_type invalid = std::numeric_limits<index_type>::max();

        private:
          vector<index_type> external2internal;
          vector<index_type> internal2external;

      };



      // Generate a fixel-fixel connectivity matrix
      init_connectivity_matrix_type generate_initial_matrix (
          const std::string& track_filename,
          Image<index_type>& index_image,
          Image<bool>& fixel_mask,
          const float angular_threshold);



      // From an initial fixel-fixel connectivity matrix, generate a
      //   "normalised" connectivity matrix, where the entries are
      //   floating-point and range from 0.0 to 1.0, and weak
      //   entries have been culled from the matrix.
      // Additionally, if required:
      //   - Convert fixel indices based on a lookup table
      //   - Generate a second connectivity matrix for the purposes
      //     of smoothing, which additionally modulates the connection
      //     weights by a spatial distance factor (and re-applies the
      //     connectivity threshold after doing so)
      // Note that this function will erase data from the input
      //   initiali connectivity matrix as it processes, in order to
      //   free up RAM for storing the output matrices.
      void normalise_matrix (
          init_connectivity_matrix_type& init_matrix,
          Image<index_type>& index_image,
          FixelIndexMapper index_mapper,
          const float connectivity_threshold,
          norm_connectivity_matrix_type& normalised_matrix,
          const float smoothing_fwhm,
          norm_connectivity_matrix_type& smoothing_matrix);




      // Template functions to save/load sparse matrices to/from file
      template <class FixelType>
      void save (vector<FixelType>& data, const std::string& filepath)
      {
        File::OFStream out (filepath);
        for (const auto& f : data) {
          for (size_t i = 0; i != f.size(); ++i) {
            if (i)
              out << ",";
            out << f[i].index() << ":" << f[i].value();
          }
          out << "\n";
        }
      }
      template <class FixelType>
      void load (const std::string& filepath, vector<FixelType>& data)
      {
        data.clear();
        std::ifstream in (filepath);
        for (std::string line; std::getline (in, line); ) {
          data.emplace_back (FixelType());
          auto entries = MR::split (line, ",");
          for (const auto& entry : entries) {
            auto pair = MR::split (entry, ":");
            if (pair.size() != 2) {
              Exception e ("Malformed sparse matrix in file \"" + filepath + "\":");
              e.push_back ("Line: \"" + line + "\"");
              e.push_back ("Entry: \"" + entry + "\"");
              throw e;
            }
            data[data.size()-1].emplace_back (FixelType (to<index_type>(pair[0]), to<typename FixelType::ValueType>(pair[1])));
          }
        }
      }





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
