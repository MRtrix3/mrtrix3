/*
    Copyright 2013 KU Leuven, Dept. Electrical Engineering, Medical Image Computing
    Herestraat 49 box 7003, 3000 Leuven, Belgium
    
    Written by Daan Christiaens, 19/03/14.
    
    This file is part of the Global Tractography module for MRtrix.
    
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

#include "gt.h"


namespace MR {
  namespace DWI {
    namespace Tractography {
      namespace GT {

        std::ostream& operator<< (std::ostream& o, Stats const& stats)
        {
          return o << stats.Tint << ", " << stats.EextTot << ", " << stats.EintTot << ": " <<
                      stats.getAcceptanceRate('b') << ", " << stats.getAcceptanceRate('d') << ", " <<
                      stats.getAcceptanceRate('r') << ", " << stats.getAcceptanceRate('o') << ", " <<
                      stats.getAcceptanceRate('c');
        }

      }
    }
  }
}

