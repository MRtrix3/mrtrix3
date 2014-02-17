#ifndef __dwi_tractography_mapping_mapping_h__
#define __dwi_tractography_mapping_mapping_h__



#include "point.h"
#include "progressbar.h"

#include "dwi/tractography/properties.h"
#include "dwi/tractography/file.h"

#include "image/header.h"

#include "math/matrix.h"

#include <vector>



namespace MR {
  namespace DWI {
    namespace Tractography {
      namespace Mapping {



#define MAX_TRACKS_READ_FOR_HEADER 1000000
        void generate_header (Image::Header&, const std::string&, const std::vector<float>&);

        void oversample_header (Image::Header&, const std::vector<float>&);





      }
    }
  }
}

#endif



