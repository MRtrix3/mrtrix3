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

#ifndef __dwi_tractography_streamline_h__
#define __dwi_tractography_streamline_h__


#include <vector>

#include "types.h"


namespace MR
{
  namespace DWI
  {
    namespace Tractography
    {


      class Streamline : public std::vector<Eigen::Vector3f>
      {
        public:
          Streamline () : index (-1), weight (1.0f) { }

          Streamline (size_t size) : 
            std::vector<Eigen::Vector3f> (size), 
            index (-1),
            weight (1.0f) { }

          Streamline (const Streamline& that) :
            std::vector<Eigen::Vector3f> (that),
            index (that.index),
            weight (that.weight) { }

          Streamline (const std::vector<Eigen::Vector3f>& tck) :
            std::vector<Eigen::Vector3f> (tck),
            index (-1),
            weight (1.0) { }

          void clear()
          {
            std::vector<Eigen::Vector3f>::clear();
            index = -1;
            weight = 1.0;
          }

          size_t index;
          float weight;
      };



    }
  }
}


#endif

