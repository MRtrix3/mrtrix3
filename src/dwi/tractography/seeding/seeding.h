#ifndef __dwi_tractography_seeding_seeding_h__
#define __dwi_tractography_seeding_seeding_h__


#include "dwi/tractography/seeding/basic.h"
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


        extern const App::OptionGroup SeedOption;
        void load_tracking_seeds (Properties&);



      }
    }
  }
}

#endif

