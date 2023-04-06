/* Copyright (c) 2008-2021 the MRtrix3 contributors.
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

#include "dwi/tractography/tracking/types.h"


namespace MR
{
  namespace DWI
  {
    namespace Tractography
    {
      namespace Tracking
      {



        const std::string termination_strings[] = {
          "Continue",
          "Entered cortical grey matter",
          "Calibrator sub-threshold",
          "Exited image",
          "Entered CSF",
          "Diffusion model sub-threshold",
          "Excessive curvature",
          "Max length exceeded",
          "Terminated in subcortex",
          "Exiting sub-cortical GM",
          "Exited mask",
          "Entered exclusion region",
          "Traversed all include regions" };



        const std::string rejection_strings[] = {
          "Invalid seed point",
          "No propagation from seed",
          "Shorter than minimum length",
          "Longer than maximum length",
          "Entered exclusion region",
          "Missed inclusion region",
          "Poor structural termination",
          "Failed to traverse white matter" };


      }
    }
  }
}

