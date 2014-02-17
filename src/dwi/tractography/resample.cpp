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

#include "dwi/tractography/resample.h"


namespace MR {
namespace DWI {
namespace Tractography {




bool Downsampler::operator() (Tracking::GeneratedTrack& tck) const
{
  if (ratio <= 1 || tck.empty())
    return false;
  size_t index_old = ratio;
  if (tck.get_seed_index()) {
    index_old = (((tck.get_seed_index() - 1) % ratio) + 1);
    tck.set_seed_index (1 + ((tck.get_seed_index() - index_old) / ratio));
  }
  size_t index_new = 1;
  while (index_old < tck.size() - 1) {
    tck[index_new++] = tck[index_old];
    index_old += ratio;
  }
  tck[index_new] = tck.back();
  tck.resize (index_new + 1);
  return true;
}





}
}
}


