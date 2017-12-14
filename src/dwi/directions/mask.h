/* Copyright (c) 2008-2017 the MRtrix3 contributors.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, you can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * MRtrix is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
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
