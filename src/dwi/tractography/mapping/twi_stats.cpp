/* Copyright (c) 2008-2019 the MRtrix3 contributors.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Covered Software is provided under this License on an "as is"
 * basis, without warranty of any kind, either expressed, implied, or
 * statutory, including, without limitation, warranties that the
 * Covered Software is free of defects, merchantable, fit for a
 * particular purpose or non-infringing.
 * See the Mozilla Public License v. 2.0 for more details.
 *
 * For more details, see http://www.mrtrix.org/.
 */

#include "dwi/tractography/mapping/twi_stats.h"


namespace MR {
namespace DWI {
namespace Tractography {
namespace Mapping {


const char* contrasts[] = { "tdi", "length", "invlength", "scalar_map", "scalar_map_count", "fod_amp", "curvature", "vector_file", 0 };
const char* voxel_statistics[] = { "sum", "min", "mean", "max", 0 };
// Note: ENDS_CORR not provided as a command-line option
const char* track_statistics[] = { "sum", "min", "mean", "max", "median", "mean_nonzero", "gaussian", "ends_min", "ends_mean", "ends_max", "ends_prod", 0 };


}
}
}
}



