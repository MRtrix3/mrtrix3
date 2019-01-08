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


#ifndef __fixel_matrix_h__
#define __fixel_matrix_h__

#include "image.h"
#include "types.h"
#include "file/ofstream.h"
#include "fixel/index_remapper.h"

namespace MR
{
  namespace Fixel
  {


    namespace Matrix
    {


      using index_type = uint32_t;
      using connectivity_value_type = float;



      // Classes for dealing with dynamic multi-threaded construction of the
      //   fixel-fixel connectivity matrix
      class InitElement
      { NOMEMALIGN
        public:
          using ValueType = index_type;
          InitElement() :
              fixel_index (std::numeric_limits<index_type>::max()),
              track_count (0) { }
          InitElement (const index_type fixel_index) :
              fixel_index (fixel_index),
              track_count (1) { }
          InitElement (const index_type fixel_index, const ValueType track_count) :
              fixel_index (fixel_index),
              track_count (track_count) { }
          InitElement (const InitElement&) = default;
          FORCE_INLINE InitElement& operator++() { track_count++; return *this; }
          FORCE_INLINE InitElement& operator= (const InitElement& that) { fixel_index = that.fixel_index; track_count = that.track_count; return *this; }
          FORCE_INLINE index_type index() const { return fixel_index; }
          FORCE_INLINE ValueType value() const { return track_count; }
          FORCE_INLINE bool operator< (const InitElement& that) const { return fixel_index < that.fixel_index; }
        private:
          index_type fixel_index;
          ValueType track_count;
      };



      class InitFixel : public vector<InitElement>
      { MEMALIGN(InitFixel)
        public:
          using ElementType = InitElement;
          using BaseType = vector<InitElement>;
          InitFixel() :
              track_count (0) { }
          void add (const vector<index_type>& indices);
          index_type count() const { return track_count; }
        private:
          index_type track_count;
      };






      // A class to store fixel index / connectivity value pairs
      //   only after the connectivity matrix has been thresholded / normalised
      class NormElement
      { NOMEMALIGN
        public:
          using ValueType = connectivity_value_type;
          NormElement (const index_type fixel_index,
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
      class NormFixel : public vector<NormElement>
      { MEMALIGN(NormFixel)
        public:
          using ElementType = NormElement;
          using BaseType = vector<NormElement>;
          NormFixel() :
            norm_multiplier (connectivity_value_type (1)) { }
          NormFixel (const BaseType& i) :
              BaseType (i),
              norm_multiplier (connectivity_value_type (1)) { }
          NormFixel (BaseType&& i) :
              BaseType (std::move (i)),
              norm_multiplier (connectivity_value_type (1)) { }
          void normalise() {
            norm_multiplier = connectivity_value_type (0);
            for (const auto& c : *this)
              norm_multiplier += c.value();
            norm_multiplier = connectivity_value_type (1) / norm_multiplier;
          }
          connectivity_value_type norm_multiplier;
      };



      // Different types are used depending on whether the connectivity matrix
      //   is in the process of being built, or whether it has been normalised
      using init_matrix_type = vector<InitFixel>;
      using norm_matrix_type = vector<NormFixel>;



      // Generate a fixel-fixel connectivity matrix
      init_matrix_type generate (
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
      void normalise (
          init_matrix_type& init_matrix,
          Image<index_type>& index_image,
          IndexRemapper index_mapper,
          const float connectivity_threshold,
          norm_matrix_type& normalised_matrix,
          const float smoothing_fwhm,
          norm_matrix_type& smoothing_matrix);




      // Template functions to save/load sparse matrices to/from file
      template <class FixelType>
      void save (vector<FixelType>& data, const std::string& filepath)
      {
        File::OFStream out (filepath);
        ProgressBar progress ("Saving fixel-fixel connectivity matrix to file \"" + filepath + "\"", data.size());
        for (const auto& f : data) {
          for (size_t i = 0; i != f.size(); ++i) {
            if (i)
              out << ",";
            out << f[i].index() << ":" << f[i].value();
          }
          out << "\n";
          ++progress;
        }
      }

      template <class FixelType>
      void load (const std::string& filepath, vector<FixelType>& data)
      {
        data.clear();
        std::ifstream in (filepath);
        ProgressBar progress ("Loading fixel-fixel connectivity matrix from file \"" + filepath + "\"");
        for (std::string line; std::getline (in, line); ) {
          data.emplace_back (FixelType());
          auto entries = MR::split (line, ",");
          for (const auto& entry : entries) {
            auto pair = MR::split (entry, ":");
            if (pair.size() != 2) {
              Exception e ("Malformed sparse matrix in file \"" + filepath + "\": does not consist of comma-separated pair");
              e.push_back ("Line: \"" + line + "\"");
              e.push_back ("Entry: \"" + entry + "\"");
              throw e;
            }
            try {
              data[data.size()-1].emplace_back (typename FixelType::ElementType (to<index_type>(pair[0]), to<typename FixelType::ElementType::ValueType>(pair[1])));
            } catch (Exception& e) {
              e.push_back ("Malformed sparse matrix in file \"" + filepath + "\": could not convert comma-separated pair to numerical values");
              e.push_back ("Line: \"" + line + "\"");
              e.push_back ("Entry: \"" + entry + "\"");
              throw e;
            }
          }
          ++progress;
        }
      }

      template <class FixelType>
      void load (const std::string& filepath, const IndexRemapper& index_remapper, vector<FixelType>& data)
      {
        data.clear();
        std::ifstream in (filepath);
        index_type counter = 0;
        ProgressBar progress ("Loading fixel-fixel connectivity matrix \"" + filepath + "\"");
        for (std::string line; std::getline (in, line); ) {
          data.emplace_back (FixelType());
          const index_type internal_index = index_remapper.e2i (counter);
          if (internal_index != index_remapper.invalid) {
            auto entries = MR::split (line, ",");
            for (const auto& entry : entries) {
              auto pair = MR::split (entry, ":");
              if (pair.size() != 2) {
                Exception e ("Malformed sparse matrix in file \"" + filepath + "\": does not consist of comma_separated pair");
                e.push_back ("Line: \"" + line + "\"");
                e.push_back ("Entry: \"" + entry + "\"");
                throw e;
              }
              try {
                const index_type external_index = to<index_type>(pair[0]);
                const typename FixelType::ElementType::ValueType value = to<typename FixelType::ElementType::ValueType>(pair[1]);
                data[data.size()-1].emplace_back (typename FixelType::ElementType (index_remapper.e2i (external_index), value));
              } catch (Exception& e) {
                e.push_back ("Malformed sparse matrix in file \"" + filepath + "\": could not convert comma-separated pair to numerical values");
                e.push_back ("Line: \"" + line + "\"");
                e.push_back ("Entry: \"" + entry + "\"");
                throw e;
              }
            }
          }
          ++counter;
          ++progress;
        }
      }


    }
  }
}

#endif
