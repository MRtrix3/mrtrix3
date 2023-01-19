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


#ifndef __fixel_index_remapper_h__
#define __fixel_index_remapper_h__

#include <limits>

#include "image.h"
#include "types.h"
#include "fixel/types.h"

namespace MR
{
  namespace Fixel
  {



    class IndexRemapper
    { NOMEMALIGN
      public:
        IndexRemapper () : mapping_is_default (true) { }
        IndexRemapper (const index_type num_fixels);
        IndexRemapper (Image<bool> fixel_mask);

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

        bool is_default() const { return mapping_is_default; }

        static constexpr index_type invalid = std::numeric_limits<index_type>::max();

      private:
        bool mapping_is_default;
        vector<index_type> external2internal;
        vector<index_type> internal2external;

    };



  }
}

#endif
