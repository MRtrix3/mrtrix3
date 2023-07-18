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

#ifndef __dwi_tractography_tracking_tractography_h__
#define __dwi_tractography_tracking_tractography_h__

#include "app.h"
#include "mrtrix.h"
#include "dwi/tractography/properties.h"


namespace MR
{
  namespace App { class OptionGroup; }

  namespace DWI
  {

    namespace Tractography
    {

      namespace Tracking
      {

        namespace Defaults
        {
          constexpr size_t num_selected_tracks = 5000;
          constexpr size_t seed_to_select_ratio = 1000;
          constexpr size_t max_attempts_per_seed = 1000;

          constexpr float cutoff_fod = 0.10f;
          constexpr float cutoff_fixel = 0.10f;
          constexpr float cutoff_fa = 0.10f;
          constexpr float cutoff_act_multiplier = 0.5f;

          constexpr size_t max_trials_per_step = 1000;

          constexpr float stepsize_voxels_firstorder = 0.1f;
          constexpr float stepsize_voxels_rk4 = 0.25f;
          constexpr float stepsize_voxels_ifod2 = 0.5f;

          constexpr float angle_deterministic = 60.0f;
          constexpr float angle_ifod1 = 15.0f;
          constexpr float angle_ifod2 = 45.0f;

          constexpr float minlength_voxels_noact = 5.0f;
          constexpr float minlength_voxels_withact = 2.0f;
          constexpr float maxlength_voxels = 100.0f;

          constexpr size_t ifod2_nsamples = 4;
        }

        extern const App::OptionGroup TrackOption;

        void load_streamline_properties_and_rois (Properties&);

      }
    }
  }
}

#endif

