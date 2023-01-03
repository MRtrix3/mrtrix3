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

#ifndef __dwi_directions_mask_h__
#define __dwi_directions_mask_h__


#include <fstream>

#include "misc/bitset.h"

#include "dwi/directions/set.h"



namespace MR {
  namespace DWI {
    namespace Directions {



      class Mask : public BitSet { NOMEMALIGN

        public:
          Mask (const Set& master, const bool allocator = false) :
              BitSet (master.size(), allocator),
              dirs (&master) { }

          Mask (const Mask& that) :
              BitSet (that),
              dirs (that.dirs) { }

          const Set& get_dirs() const { return *dirs; }

          void erode  (const size_t iterations = 1);
          void dilate (const size_t iterations = 1);

          bool is_adjacent (const size_t) const;
          size_t get_min_linkage (const Mask&);

        private:
          const Set* dirs;

      };



    }
  }
}

#endif
