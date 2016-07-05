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

#ifndef __fixel_helpers_h__
#define __fixel_helpers_h__

#include "formats/mrtrix_utils.h"
#include "fixel_format/keys.h"

namespace MR
{
  namespace FixelFormat
  {
    template <class IndexFileHeader>
    inline bool is_index_file (const IndexFileHeader& in)
    {
      return in.keyval ().count (n_fixels_key);
    }


    template <class IndexFileHeader>
    inline void check_index_file (const IndexFileHeader& in)
    {
      if (!is_index_file (in))
        throw Exception (in.name () + " is not a valid fixel index file. Header key " + n_fixels_key + " not found");
    }


    template <class IndexFileHeader, class DataFileHeader>
    inline bool fixels_match (const IndexFileHeader& index_h, const DataFileHeader& data_h)
    {
      bool fixels_match (false);

      if (is_index_file (index_h)) {
        fixels_match = std::stoul(index_h.keyval ().at (n_fixels_key)) == (unsigned long)data_h.size (0);
      }

      return fixels_match;
    }


    template <class IndexFileHeader, class DataFileHeader>
    inline void check_fixel_size (const IndexFileHeader& index_h, const DataFileHeader& data_h)
    {
      check_index_file (index_h);

      if (!fixels_match (index_h, data_h))
        throw Exception ("Fixel number mismatch between index file " + index_h.name() + " and data file " + data_h.name());
    }
  }
}

#endif

