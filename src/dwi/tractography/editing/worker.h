/* Copyright (c) 2008-2017 the MRtrix3 contributors
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


#ifndef __dwi_tractography_editing_worker_h__
#define __dwi_tractography_editing_worker_h__


#include <string>
#include <vector>

#include "dwi/tractography/properties.h"
#include "dwi/tractography/streamline.h"



namespace MR {
  namespace DWI {
    namespace Tractography {
      namespace Editing {




        class Worker
        { MEMALIGN(Worker)

          public:
            Worker (Tractography::Properties& p, const bool inv, const bool end) :
              properties (p),
              inverse (inv),
              ends_only (end),
              thresholds (p),
              include_visited (properties.include.size(), false) { }

            Worker (const Worker& that) :
              properties (that.properties),
              inverse (that.inverse),
              ends_only (that.ends_only),
              thresholds (that.thresholds),
              include_visited (properties.include.size(), false) { }


            bool operator() (Streamline<>&, Streamline<>&) const;


          private:
            const Tractography::Properties& properties;
            const bool inverse, ends_only;

            class Thresholds
            { NOMEMALIGN
              public:
                Thresholds (Tractography::Properties&);
                Thresholds (const Thresholds&);
                bool operator() (const Streamline<>&) const;
              private:
                float max_length, min_length;
                float max_weight, min_weight;
                float step_size;
            } thresholds;

            mutable vector<bool> include_visited;

        };




      }
    }
  }
}

#endif
