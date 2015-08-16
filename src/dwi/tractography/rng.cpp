#include "dwi/tractography/rng.h"

namespace MR
{
  namespace DWI
  {
    namespace Tractography
    {

#ifdef MRTRIX_MACOSX
      __thread Math::RNG* rng = nullptr;
#else
      thread_local Math::RNG* rng = nullptr;
#endif 

    }
  }
}

