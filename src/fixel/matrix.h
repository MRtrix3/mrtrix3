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






      // Different types are used depending on whether the connectivity matrix
      //   is in the process of being built, or whether it has been normalised
      // TODO Revise
      using init_matrix_type = vector<InitFixel>;






      // Generate a fixel-fixel connectivity matrix
      init_matrix_type generate (
          const std::string& track_filename,
          Image<index_type>& index_image,
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
                                const std::string& path);



      // TODO Wrapper class for reading the connectivity matrix



    }
  }
}

#endif
