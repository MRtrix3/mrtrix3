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
      // Members are:
      // * 'dir': orientation of fixel on unit vector xyz triplet
      // * 'size': parameter related to the size of the fixel (e.g. FOD lobe integral, bolume fraction, FOD peak amplitude)
      // * 'value': the parameteric value of interest associated with the fixel
      class FixelMetric
      {
        public:
          FixelMetric (const Point<float>& d, const float s, const float v) :
            dir (d),
            size (s),
            value (v) { }
          FixelMetric () :
            dir (),
            size (0.0),
            value (0.0) { }
          Point<float> dir;
          float size;
          float value;
      };



    }
  }
}

#endif




