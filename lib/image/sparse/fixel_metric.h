/*
    Copyright 2008 Brain Research Institute, Melbourne, Australia

    Written by Robert E. Smith, 02/08/13.

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

#ifndef __image_sparse_fixel_metric_h__
#define __image_sparse_fixel_metric_h__

#include "point.h"

namespace MR
{
  namespace Image
  {
    namespace Sparse
    {



      // A class for storing a single quantitative value per fixel
      // This simple class will form the basis of most fixel-based image outputs and statistical analysis
      // Decided to include an 'amplitude' field in here; when parameters other than the AFD are
      //   tested, it may be useful from a visualisation perspective to be able to e.g. scale fixels by
      //   AFD but colour by the parameter of interest. It may also be a useful parameter in the process
      //   of determining fixel correspondence, or adding/averaging quantities between multiple fixels.
      class FixelMetric
      {
        public:
          FixelMetric (const Point<float>& d, const float a, const float v) :
            dir (d),
            amplitude (a),
            value (v) { }
          FixelMetric () :
            dir (),
            amplitude (0.0),
            value (0.0) { }
          Point<float> dir;
          float amplitude, value;
      };



    }
  }
}

#endif




