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

#ifndef __fixelformat_loop_h__
#define __fixelformat_loop_h__

#include "formats/mrtrix_utils.h"

namespace MR
{
  namespace FixelFormat {


    namespace {
      struct set_offset {
        FORCE_INLINE set_offset (u_int32_t offset) : offset (offset) { }
        template <class DataType>
          FORCE_INLINE void operator() (DataType& data) { data.index(0) = offset; }
        u_int32_t offset;
      };

      struct inc_fixel {
        template <class DataType>
          FORCE_INLINE void operator() (DataType& data) { ++data.index(0); }
      };
    }

    struct LoopFixelsInVoxel {
      const size_t num_fixels;
      const uint32_t offset;

      template <class... DataType>
      struct Run {
        const size_t num_fixels;
        const uint32_t offset;
        uint32_t fixel_index;
        const std::tuple<DataType&...> data;
        FORCE_INLINE Run (const size_t num_fixels, const uint32_t offset, const std::tuple<DataType&...>& data) :
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
      FixelLoop (IndexType& index) {
        index.index(3) = 0;
        size_t num_fixels = index.value();
        index.index(3) = 1;
        uint32_t offset = index.value();
        return { num_fixels, offset };
      }

  }
}

#endif
