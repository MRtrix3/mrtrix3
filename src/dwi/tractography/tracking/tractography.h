/*
 * Copyright (c) 2008-2018 the MRtrix3 contributors.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, you can obtain one at http://mozilla.org/MPL/2.0/
 *
 * MRtrix3 is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * For more details, see http://www.mrtrix.org/
 */


#ifndef __dwi_tractography_tracking_tractography_h__
#define __dwi_tractography_tracking_tractography_h__

#include "app.h"
#include "mrtrix.h"
#include "dwi/tractography/properties.h"


#define TCKGEN_DEFAULT_NUM_SELECTED_TRACKS 5000
#define TCKGEN_DEFAULT_SEED_TO_SELECT_RATIO 1000
#define TCKGEN_DEFAULT_MAX_ATTEMPTS_PER_SEED 1000
#define TCKGEN_DEFAULT_CUTOFF_FOD 0.05
#define TCKGEN_DEFAULT_CUTOFF_FIXEL 0.05
#define TCKGEN_DEFAULT_CUTOFF_FA 0.10
#define TCKGEN_DEFAULT_MAX_TRIALS_PER_STEP 1000



namespace MR
{
  namespace App { class OptionGroup; }

  namespace DWI
  {

    namespace Tractography
    {

      namespace Tracking
      {

        extern const App::OptionGroup TrackOption;

        void load_streamline_properties_and_rois (Properties&);

      }
    }
  }
}

#endif

