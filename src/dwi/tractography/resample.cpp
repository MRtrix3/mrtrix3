#include "dwi/tractography/resample.h"


namespace MR {
namespace DWI {
namespace Tractography {




bool Downsampler::operator() (Tracking::GeneratedTrack& tck) const
{
  if (ratio <= 1 || tck.empty())
    return false;
  size_t index_old = ratio;
  if (tck.get_seed_index()) {
    index_old = (((tck.get_seed_index() - 1) % ratio) + 1);
    tck.set_seed_index (1 + ((tck.get_seed_index() - index_old) / ratio));
  }
  size_t index_new = 1;
  while (index_old < tck.size() - 1) {
    tck[index_new++] = tck[index_old];
    index_old += ratio;
  }
  tck[index_new] = tck.back();
  tck.resize (index_new + 1);
  return true;
}





}
}
}


