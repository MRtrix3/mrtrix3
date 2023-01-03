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

#ifndef __fixel_loop_h__
#define __fixel_loop_h__

#include "fixel/types.h"
#include "formats/mrtrix_utils.h"

namespace MR
{
  namespace Fixel {


    namespace {
      struct set_offset { NOMEMALIGN
        FORCE_INLINE set_offset (index_type offset) : offset (offset) { }
        template <class DataType>
          FORCE_INLINE void operator() (DataType& data) { data.index(0) = offset; }
        index_type offset;
      };

      struct inc_fixel { NOMEMALIGN
        template <class DataType>
          FORCE_INLINE void operator() (DataType& data) { ++data.index(0); }
      };
    }

    struct LoopFixelsInVoxel { NOMEMALIGN
      const index_type num_fixels;
      const index_type offset;

      template <class... DataType>
      struct Run { NOMEMALIGN
        const index_type num_fixels;
        const index_type offset;
        index_type fixel_index;
        const std::tuple<DataType&...> data;
        FORCE_INLINE Run (const index_type num_fixels, const index_type offset, const std::tuple<DataType&...>& data) :
          num_fixels (num_fixels), offset (offset), fixel_index (0), data (data) {
          apply (set_offset (offset), data);
        }
        FORCE_INLINE operator bool() const { return fixel_index < num_fixels; }
        FORCE_INLINE void operator++() { apply (inc_fixel (), data); fixel_index++; }
        FORCE_INLINE void operator++(int) const { operator++(); }
      };

      template <class... DataType>
        FORCE_INLINE Run<DataType...> operator() (DataType&... data) const { return { num_fixels, offset, std::tie (data...) }; }

    };

    template <class IndexType>
      FORCE_INLINE LoopFixelsInVoxel
      Loop (IndexType& index) {
        index.index(3) = 0;
        index_type num_fixels = index.value();
        index.index(3) = 1;
        index_type offset = index.value();
        return { num_fixels, offset };
      }
  }
}

#endif
