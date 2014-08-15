/*
    Copyright 2011 Brain Research Institute, Melbourne, Australia

    Written by Robert E. Smith, 2014.

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


#include "dwi/tractography/mapping/gaussian/mapper.h"


namespace MR {
namespace DWI {
namespace Tractography {
namespace Mapping {
namespace Gaussian {




void TrackMapper::set_factor (const Streamline<>& tck, SetVoxelExtras& out) const
{
  factors.clear();
  factors.reserve (tck.size());
  load_factors (tck);
  gaussian_smooth_factors (tck);
  out.factor = 1.0;
}




void TrackMapper::gaussian_smooth_factors (const Streamline<>& tck) const
{

  std::vector<float> unsmoothed (factors);

  for (size_t i = 0; i != unsmoothed.size(); ++i) {

    double sum = 0.0, norm = 0.0;

    if (std::isfinite (unsmoothed[i])) {
      sum  = unsmoothed[i];
      norm = 1.0; // Gaussian is unnormalised -> e^0 = 1
    }

    float distance = 0.0;
    for (size_t j = i; j--; ) { // Decrement AFTER null test, so loop runs with j = 0
      distance += dist(tck[j], tck[j+1]);
      if (std::isfinite (unsmoothed[j])) {
        const float this_weight = exp (-distance * distance / gaussian_denominator);
        norm += this_weight;
        sum  += this_weight * unsmoothed[j];
      }
    }
    distance = 0.0;
    for (size_t j = i + 1; j < unsmoothed.size(); ++j) {
      distance += dist(tck[j], tck[j-1]);
      if (std::isfinite (unsmoothed[j])) {
        const float this_weight = exp (-distance * distance / gaussian_denominator);
        norm += this_weight;
        sum  += this_weight * unsmoothed[j];
      }
    }

    if (norm)
      factors[i] = (sum / norm);
    else
      factors[i] = 0.0;

  }

}







}
}
}
}
}



