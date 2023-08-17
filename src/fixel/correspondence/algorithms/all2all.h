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

#ifndef __fixel_correspondence_algorithms_all2all_h__
#define __fixel_correspondence_algorithms_all2all_h__


#include "types.h"

#include "fixel/correspondence/algorithms/base.h"
#include "fixel/correspondence/fixel.h"


namespace MR {
  namespace Fixel {
    namespace Correspondence {
      namespace Algorithms {



#ifdef FIXELCORRESPONDENCE_INCLUDE_ALL2ALL
        // For the sake of testing, construct a correspondence algorithm with predictable behaviour:
        //   assign all source fixels to every target fixel
        class All2All : public Base
        {
          public:
            All2All() {}
            virtual ~All2All() {}
            vector< vector<uint32_t> > operator() (const voxel_t&,
                                                   const vector<Correspondence::Fixel>& s,
                                                   const vector<Correspondence::Fixel>& t) const final
            {
              vector< vector<uint32_t> > result;
              vector<uint32_t> all_s;
              for (uint32_t i = 0; i != s.size(); ++i)
                all_s.push_back (i);
              result.assign (t.size(), all_s);
              return result;
            }
        };
#endif



      }
    }
  }
}

#endif
