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


      using index_image_type = uint64_t;
      using fixel_index_type = uint32_t;
      using count_type = uint32_t;
      using connectivity_value_type = float;



      // Classes for dealing with dynamic multi-threaded construction of the
      //   fixel-fixel connectivity matrix
      class InitElement
      { NOMEMALIGN
        public:
          using ValueType = fixel_index_type;
          InitElement() :
              fixel_index (std::numeric_limits<fixel_index_type>::max()),
              track_count (0) { }
          InitElement (const fixel_index_type fixel_index) :
              fixel_index (fixel_index),
              track_count (1) { }
          InitElement (const fixel_index_type fixel_index, const ValueType track_count) :
              fixel_index (fixel_index),
              track_count (track_count) { }
          InitElement (const InitElement&) = default;
          FORCE_INLINE InitElement& operator++() { track_count++; return *this; }
          FORCE_INLINE InitElement& operator= (const InitElement& that) { fixel_index = that.fixel_index; track_count = that.track_count; return *this; }
          FORCE_INLINE fixel_index_type index() const { return fixel_index; }
          FORCE_INLINE ValueType value() const { return track_count; }
          FORCE_INLINE bool operator< (const InitElement& that) const { return fixel_index < that.fixel_index; }
        private:
          fixel_index_type fixel_index;
          ValueType track_count;
      };



      class InitFixel : public vector<InitElement>
      { MEMALIGN(InitFixel)
        public:
          using ElementType = InitElement;
          using BaseType = vector<InitElement>;
          InitFixel() :
              track_count (0) { }
          void add (const vector<fixel_index_type>& indices);
          count_type count() const { return track_count; }
        private:
          count_type track_count;
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
            norm_multiplier = norm_multiplier ? (connectivity_value_type (1) / norm_multiplier) : connectivity_value_type(0);
          }
          void normalise (const connectivity_value_type sum) {
            norm_multiplier = sum ? (connectivity_value_type(1) / sum) : connectivity_value_type(0);
          }
          connectivity_value_type norm_multiplier;
      };






      // Different types are used depending on whether the connectivity matrix
      //   is in the process of being built, or whether it has been normalised
      // TODO Revise
      using init_matrix_type = vector<InitFixel>;






      // Generate a fixel-fixel connectivity matrix
      init_matrix_type generate (
          const std::string& track_filename,
          Image<fixel_index_type>& index_image,
          Image<bool>& fixel_mask,
          const float angular_threshold);






      // New code for handling load/save of fixel-fixel connectivity matrix
      // Use something akin to the fixel directory format, with a sub-directory
      //   within an existing fixel directory containing the following data:
      // - index.mif: Similar to the index image in the fixel directory format,
      //   but has dimensions Nx1x1x2, where:
      //     - N is the number of fixels in the fixel template
      //     - The first volume contains the number of connected fixels for that fixel
      //     - The second volume contains the offset of the first connected fixel for that fixel
      // - connectivity.mif: Floating-point image of dimension Cx1x1, where C is the total
      //   number of fixel-fixel connections stored in the entire matrix. Each value should be
      //   between 0.0 and 1.0, corresponding to the fraction of streamlines passing through
      //   the fixel that additionally pass through some other fixel.
      // - fixel.mif: Unsigned integer image of dimension Cx1x1, where C is the total number of
      //   fixel-fixel connections stored in the entire matrix. Each value indexes the
      //   fixel to which the fixel-fixel connection refers.
      //
      // In order to avoid duplication of memory usage, the writing function should
      //   perform, for each fixel in turn:
      // - Normalisation of the matrix
      // - Writing to the three images
      // - Erasing the memory used for that matrix in the initial building

      void normalise_and_write (init_matrix_type& matrix,
                                const connectivity_value_type threshold,
                                const std::string& path,
                                const KeyValues& keyvals = KeyValues());



      // Wrapper class for reading the connectivity matrix from the filesystem
      class Reader
      { MEMALIGN(Reader)

        public:
          Reader (const std::string& path, const Image<bool>& mask);
          Reader (const std::string& path);

          // TODO Entirely feasible to construct this thing using scratch storage;
          //   would need two passes over the pre-normalised data in order to calculate
          //   the number of fixel-fixel connections, but it could be done
          //
          // It would require restoration of the old Matrix::normalise() function,
          //   but modification to write out to scratch index / fixel / value images
          //   rather than "norm_matrix_type"
          //
          // This would permit usage of fixelcfestats with tractogram as input
          //
          // TODO Could pre-exponentiation of connectivity values be done beforehand using an mrcalc call?
          // Expect fixelcfestats to be provided with a data file, from which it will find the
          //   index & fixel images

          NormFixel operator[] (const size_t index) const;

          // TODO Define iteration constructs?

          size_t size() const { return index_image.size (0); }
          size_t size (const size_t) const;

        protected:
          const std::string directory;
          // Not to be manipulated directly; need to copy in order to ensure thread-safety
          Image<index_image_type> index_image;
          Image<fixel_index_type> fixel_image;
          Image<connectivity_value_type> value_image;
          Image<bool> mask_image;


      };



    }
  }
}

#endif
