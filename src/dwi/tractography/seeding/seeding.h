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

#ifndef __dwi_tractography_seeding_seeding_h__
#define __dwi_tractography_seeding_seeding_h__


#include "mrtrix.h"
#include "dwi/tractography/seeding/basic.h"
#include "dwi/tractography/seeding/dynamic.h"
#include "dwi/tractography/seeding/gmwmi.h"
#include "dwi/tractography/seeding/list.h"



namespace MR
{

  namespace App { class OptionGroup; }

  namespace DWI
  {
    namespace Tractography
    {

      class Properties;

      namespace Seeding
      {


        extern const App::OptionGroup SeedMechanismOption;
        extern const App::OptionGroup SeedParameterOption;
        void load_seed_mechanisms (Properties&);
        void load_seed_parameters (Properties&);



      }
    }
  }
}

#endif

