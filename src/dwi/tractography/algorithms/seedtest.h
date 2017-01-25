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


#ifndef __dwi_tractography_algorithms_seedtest_h__
#define __dwi_tractography_algorithms_seedtest_h__

#include "dwi/tractography/tracking/method.h"
#include "dwi/tractography/tracking/shared.h"
#include "dwi/tractography/tracking/types.h"



namespace MR {
namespace DWI {
namespace Tractography {
namespace Algorithms {

using namespace MR::DWI::Tractography::Tracking;

class Seedtest : public MethodBase {

  public:

  class Shared : public SharedBase {
    public:
    Shared (const std::string& diff_path, DWI::Tractography::Properties& property_set) :
        SharedBase (diff_path, property_set)
    {
      set_step_size (1.0);
      min_num_points = 1;
      max_num_points = 2;
      unidirectional = true;
      properties["method"] = "Seedtest";
    }
  };

  Seedtest (const Shared& shared) :
    MethodBase (shared),
    S (shared) { }


  bool init() { return true; }
  term_t next () { return EXIT_IMAGE; }
  float get_metric() { return 1.0; }


  protected:
    const Shared& S;

};

}
}
}
}

#endif


