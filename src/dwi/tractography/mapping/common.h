/*
    Copyright 2011 Brain Research Institute, Melbourne, Australia

    Written by Robert E. Smith, 2011.

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

#ifndef __dwi_tractography_mapping_common_h__
#define __dwi_tractography_mapping_common_h__



#include "point.h"

#include "thread/queue.h"



namespace MR {
namespace DWI {
namespace Tractography {
namespace Mapping {


// Want to retain the index # of each track (not necessary for tckmap, but may be for other functions)
class TrackAndIndex {
  public:
    std::vector< Point<float> > tck;
    size_t index;
};

typedef Thread::Queue<TrackAndIndex> TrackQueue;


}
}
}
}

#endif



