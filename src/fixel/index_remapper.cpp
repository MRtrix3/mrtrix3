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


#include "fixel/index_remapper.h"

namespace MR
{
  namespace Fixel
  {



    constexpr index_type IndexRemapper::invalid;



    IndexRemapper::IndexRemapper (const index_type num_fixels)
    {
      external2internal.resize (num_fixels);
      internal2external.resize (num_fixels);
      for (index_type f = 0; f != num_fixels; ++f)
        external2internal[f] = internal2external[f] = f;
      mapping_is_default = true;
    }

    IndexRemapper::IndexRemapper (Image<bool> fixel_mask)
    {
      external2internal.resize (fixel_mask.size (0));
      index_type counter = 0;
      for (fixel_mask.index(0) = 0; fixel_mask.index(0) != fixel_mask.size(0); ++fixel_mask.index(0))
        external2internal[fixel_mask.index(0)] = fixel_mask.value() ? counter++ : invalid;
  #ifdef NDEBUG
      internal2external.resize (counter);
  #else
      internal2external.assign (counter, invalid);
  #endif
      for (index_type e = 0; e != fixel_mask.size(0); ++e) {
        if (external2internal[e] != invalid)
          internal2external[external2internal[e]] = e;
      }
  #ifndef NDEBUG
      for (index_type i = 0; i != counter; ++i)
        assert (internal2external[i] != invalid);
  #endif
      mapping_is_default = false;
    }



  }
}
