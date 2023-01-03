/* Copyright (c) 2008-2023 the MRtrix3 contributors.
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
 * See the Mozilla Public License v. 2.0 for more details.
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

class Seedtest : public MethodBase { MEMALIGN(Seedtest)

  public:

  class Shared : public SharedBase { MEMALIGN(Shared)
    public:
    Shared (const std::string& diff_path, DWI::Tractography::Properties& property_set) :
        SharedBase (diff_path, property_set)
    {
      set_step_and_angle (1.0f, 90.0f, false);
      min_num_points_preds = min_num_points_postds = 1;
      max_num_points_preds = max_num_points_postds = 2;
      set_cutoff (0.0f);
      unidirectional = true;
      properties["method"] = "Seedtest";
    }
  };

  Seedtest (const Shared& shared) :
    MethodBase (shared),
    S (shared) { }


  bool init() override { return true; }
  term_t next () override { return EXIT_IMAGE; }
  float get_metric (const Eigen::Vector3f& position, const Eigen::Vector3f& direction) override { return 1.0f; }


  protected:
    const Shared& S;

};

}
}
}
}

#endif


