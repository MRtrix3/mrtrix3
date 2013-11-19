
#include "dwi/tractography/mapping/fixel_td_map.h"



namespace MR {
namespace DWI {
namespace Tractography {
namespace Mapping {



using namespace App;

const OptionGroup FixelMapProcMaskOption = OptionGroup ("Options for setting the processing mask for the fixel-streamlines comparison model")

  + Option ("proc_mask", "provide an image containing the processing mask weights for the model; image spatial dimensions must match the fixel image")
    + Argument ("image").type_image_in()

  + Option ("act", "use an ACT four-tissue-type segmented anatomical image to derive the processing mask")
    + Argument ("image").type_image_in();


}
}
}
}


