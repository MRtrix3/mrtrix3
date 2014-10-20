#include "dwi/tractography/weights.h"

namespace MR
{
  namespace DWI
  {
    namespace Tractography
    {

      using namespace App;

      const Option TrackWeightsInOption
      = Option ("tck_weights_in", "specify a text scalar file containing the streamline weights")
          + Argument ("path").type_file_in();

      const Option TrackWeightsOutOption
      = Option ("tck_weights_out", "specify the path for an output text scalar file containing streamline weights")
          + Argument ("path").type_file_out();

    }
  }
}


