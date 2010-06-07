/*
    Copyright 2009 Brain Research Institute, Melbourne, Australia

    Written by J-Donald Tournier, 13/11/09.

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

#include "dwi/tractography/properties.h"


namespace MR {
  namespace DWI {
    namespace Tractography {

      void Properties::load_ROIs () 
      {
        typedef std::multimap<std::string,std::string>::const_iterator iter;

        std::pair<iter,iter> range = roi.equal_range ("seed");
        for (iter it = range.first; it != range.second; ++it) seed.add (it->second);
        range = roi.equal_range ("include");
        for (iter it = range.first; it != range.second; ++it) include.add (it->second);
        range = roi.equal_range ("exclude");
        for (iter it = range.first; it != range.second; ++it) exclude.add (it->second);
        range = roi.equal_range ("mask");
        for (iter it = range.first; it != range.second; ++it) mask.add (it->second);

        roi.clear();
      }

    }
  }
}

