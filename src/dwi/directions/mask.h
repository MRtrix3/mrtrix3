/*
   Copyright 2008 Brain Research Institute, Melbourne, Australia

   Written by Robert Smith, 2010.

   This file is part of MRtrix.

   MRtrix is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   MRtrix is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with MRtrix.  If not, see <http://www.gnu.org/licenses/>.

 */


#ifndef __dwi_directions_mask_h__
#define __dwi_directions_mask_h__


#include <fstream>

#include "bitset.h"
#include "dwi/directions/set.h"



namespace MR {
  namespace DWI {
    namespace Directions {



      class Mask : public BitSet {

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
