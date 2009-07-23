/*
    Copyright 2008 Brain Research Institute, Melbourne, Australia

    Written by J-Donald Tournier, 27/06/08.

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

#ifndef __histogram_h__
#define __histogram_h__

#include "mrtrix.h"

namespace MR {
  namespace Image {
    class Voxel;
  }

  class Histogram {
    public:
      Histogram (Image::Voxel& ima, uint num_buckets=100);

      uint   frequency (uint index) const    { return (list[index].frequency); }
      float   value (uint index) const        { return (list[index].value); }
      uint   num () const                    { return (list.size()); }
      float   first_min () const;

    protected:
      class Entry {
        public:
          Entry () : frequency (0), value (0.0) { }
          uint  frequency;
          float  value;
      };

      std::vector<Entry> list;

  };

}

#endif
