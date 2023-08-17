/* Copyright (c) 2008-2017 the MRtrix3 contributors.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, you can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * MRtrix is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * For more details, see http://www.mrtrix.org/.
 */

#ifndef __fixel_correspondence_algorithms_base_h__
#define __fixel_correspondence_algorithms_base_h__


#include <string>

#include "image.h"
#include "types.h"

#include "fixel/correspondence/correspondence.h"
#include "fixel/correspondence/fixel.h"


namespace MR {
  namespace Fixel {
    namespace Correspondence {
      namespace Algorithms {



        class Base
        {
          public:
            Base() {}
            ~Base() {}

            void export_cost_image (const std::string& path)
            {
              if (!cost_image.valid()) return;
              Image<float> output (Image<float>::create (path, cost_image));
              copy (cost_image, output);
            }

            virtual vector< vector<uint32_t> > operator() (const voxel_t& v,
                                                           const vector<Correspondence::Fixel>& s,
                                                           const vector<Correspondence::Fixel>& t) const = 0;
          protected:
            Image<float> cost_image;
        };



      }
    }
  }
}

#endif
