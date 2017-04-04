/*
 * Copyright (c) 2008-2016 the MRtrix3 contributors
 * 
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/
 * 
 * MRtrix is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * 
 * For more details, see www.mrtrix.org
 * 
 */

#include "dwi/tractography/properties.h"


namespace MR {
  namespace DWI {
    namespace Tractography {

      void Properties::load_ROIs ()
      {
        using iter = std::multimap<std::string,std::string>::const_iterator;

        std::pair<iter,iter> range = roi.equal_range ("include");
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

