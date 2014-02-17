#ifndef __dwi_tractography_tracking_tractography_h__
#define __dwi_tractography_tracking_tractography_h__

#include "app.h"
#include "point.h"

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

        extern const App::OptionGroup TrackOption;

        void load_streamline_properties (Properties&);

      }
    }
  }
}

#endif

