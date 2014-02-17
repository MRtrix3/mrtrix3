#include "dwi/tractography/mapping/twi_stats.h"


namespace MR {
namespace DWI {
namespace Tractography {
namespace Mapping {


const char* contrasts[] = { "tdi", "precise_tdi", "endpoint", "length", "invlength", "scalar_map", "scalar_map_count", "fod_amp", "curvature", 0 };
const char* voxel_statistics[] = { "sum", "min", "mean", "max", 0 };
const char* track_statistics[] = { "sum", "min", "mean", "max", "median", "mean_nonzero", "gaussian", "ends_min", "ends_mean", "ends_max", "ends_prod", "ends_corr", 0 };


}
}
}
}



