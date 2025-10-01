/* Copyright (c) 2008-2025 the MRtrix3 contributors.
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
#include "twi_stats.h"

#include <string>
#include <vector>

namespace MR::DWI::Tractography::Mapping {

const std::vector<std::string> contrasts = {
    "tdi", "length", "invlength", "scalar_map", "scalar_map_count", "fod_amp", "curvature", "vector_file"};
const std::vector<std::string> voxel_statistics = {"sum", "min", "mean", "max"};
// Note: ENDS_CORR not provided as a command-line option
const std::vector<std::string> track_statistics = {"sum",
                                                   "min",
                                                   "mean",
                                                   "max",
                                                   "median",
                                                   "mean_nonzero",
                                                   "gaussian",
                                                   "ends_min",
                                                   "ends_mean",
                                                   "ends_max",
                                                   "ends_prod"};

} // namespace MR::DWI::Tractography::Mapping
