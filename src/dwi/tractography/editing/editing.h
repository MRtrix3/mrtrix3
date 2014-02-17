#ifndef __dwi_tractography_editing_editing_h__
#define __dwi_tractography_editing_editing_h__

#include "app.h"

#include "dwi/tractography/properties.h"

namespace MR {
namespace DWI {
namespace Tractography {
namespace Editing {



extern const App::OptionGroup LengthOption;
extern const App::OptionGroup ResampleOption;
extern const App::OptionGroup TruncateOption;
extern const App::OptionGroup WeightsOption;


void load_properties (Tractography::Properties&);



}
}
}
}

#endif
