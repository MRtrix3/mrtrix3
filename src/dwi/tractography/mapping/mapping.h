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

#ifndef __dwi_tractography_mapping_mapping_h__
#define __dwi_tractography_mapping_mapping_h__

#include <vector>

#include "header.h"
#include "progressbar.h"
#include "dwi/tractography/properties.h"
#include "dwi/tractography/file.h"

namespace MR {
  namespace DWI {
    namespace Tractography {
      namespace Mapping {



        // Convenience functions to figure out an appropriate upsampling ratio for streamline mapping
        size_t determine_upsample_ratio (const Header&, const float, const float);
        size_t determine_upsample_ratio (const Header&, const std::string&, const float);
        size_t determine_upsample_ratio (const Header&, const Tractography::Properties&, const float);



#define MAX_TRACKS_READ_FOR_HEADER 1000000
        void generate_header (Header&, const std::string&, const std::vector<default_type>&);

        void oversample_header (Header&, const std::vector<default_type>&);





      }
    }
  }
}

#endif



