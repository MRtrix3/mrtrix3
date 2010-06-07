/*
    Copyright 2009 Brain Research Institute, Melbourne, Australia

    Written by J-Donald Tournier, 25/10/09.

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

#include "dwi/tractography/shared.h"

namespace MR {
  namespace DWI {
    namespace Tractography {

      const std::vector<ssize_t>& SharedBase::strides_by_volume ()
      {
        static std::vector<ssize_t> strides_by_vol (4);
        strides_by_vol[0] = strides_by_vol[1] = strides_by_vol[2] = 0;
        strides_by_vol[3] = 1;
        return (strides_by_vol);
      }

    }
  }
}

