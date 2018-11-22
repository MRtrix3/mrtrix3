/* Copyright (c) 2008-2019 the MRtrix3 contributors.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Covered Software is provided under this License on an "as is"
 * basis, without warranty of any kind, either expressed, implied, or
 * statutory, including, without limitation, warranties that the
 * Covered Software is free of defects, merchantable, fit for a
 * particular purpose or non-infringing.
 * See the Mozila Public License v. 2.0 for more details.
 *
 * For more details, see http://www.mrtrix.org/.
 */

#ifndef __dwi_tractography_mapping_mapping_h__
#define __dwi_tractography_mapping_mapping_h__

#include "header.h"
#include "progressbar.h"
#include "types.h"

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
        void generate_header (Header&, const std::string&, const vector<default_type>&);

        void oversample_header (Header&, const vector<default_type>&);





      }
    }
  }
}

#endif



