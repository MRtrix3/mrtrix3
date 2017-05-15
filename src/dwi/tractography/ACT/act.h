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


#ifndef __dwi_tractography_act_act_h__
#define __dwi_tractography_act_act_h__

#include "app.h"
#include "header.h"


// Actually think it's preferable to not use these
#define ACT_WM_INT_REQ 0.0
#define ACT_WM_ABS_REQ 0.0

#define GMWMI_ACCURACY 0.01 // Absolute value of tissue proportion difference

// Number of times a backtrack attempt will be made from a certain maximal track length before the length of truncation is increased
#define ACT_BACKTRACK_ATTEMPTS 3


namespace MR
{

  namespace App { class OptionGroup; }

  namespace DWI
  {

    namespace Tractography
    {

      class Properties;

      namespace ACT
      {

        extern const App::OptionGroup ACTOption;

        void load_act_properties (Properties& properties);

        void verify_5TT_image (const Header&);


      }
    }
  }
}

#endif

