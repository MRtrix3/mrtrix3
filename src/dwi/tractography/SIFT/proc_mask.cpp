
#include "dwi/tractography/SIFT/proc_mask.h"


#include "point.h"


namespace MR {
namespace DWI {
namespace Tractography {
namespace SIFT {



using namespace App;

const OptionGroup SIFTModelProcMaskOption = OptionGroup ("Options for setting the processing mask for the SIFT fixel-streamlines comparison model")

  + Option ("proc_mask", "provide an image containing the processing mask weights for the model; image spatial dimensions must match the fixel image")
    + Argument ("image").type_image_in()

  + Option ("act", "use an ACT four-tissue-type segmented anatomical image to derive the processing mask")
    + Argument ("image").type_image_in();




}
}
}
}


