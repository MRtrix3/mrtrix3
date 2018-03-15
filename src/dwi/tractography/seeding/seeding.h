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


#ifndef __dwi_tractography_seeding_seeding_h__
#define __dwi_tractography_seeding_seeding_h__


#include "mrtrix.h"
#include "dwi/tractography/seeding/basic.h"
#include "dwi/tractography/seeding/dynamic.h"
#include "dwi/tractography/seeding/gmwmi.h"
#include "dwi/tractography/seeding/list.h"
#include "dwi/tractography/seeding/mesh.h"



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

