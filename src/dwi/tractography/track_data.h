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

#ifndef __dwi_tractography_track_data_h__
#define __dwi_tractography_track_data_h__


#include <vector>

#include "point.h"


namespace MR
{
  namespace DWI
  {
    namespace Tractography
    {


      template <typename T = float>
      class TrackData : public std::vector< Point<T> >
      {
        public:
          typedef T value_type;
          TrackData () :
              std::vector< Point<value_type> > (),
              index (-1),
              weight (value_type (1.0)) { }
          TrackData (const std::vector< Point<value_type> >& tck) :
              std::vector< Point<value_type> > (tck),
              index (-1),
              weight (1.0) { }
          size_t index;
          value_type weight;
      };



    }
  }
}


#endif

