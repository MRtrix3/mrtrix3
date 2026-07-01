/* Copyright (c) 2008-2026 the MRtrix3 contributors.
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

#include "dwi/tractography/selection_dps.h"

#include "file/matrix.h"
#include "types.h"

namespace MR::DWI::Tractography {

void write_selection_dps(const std::filesystem::path &path, const std::vector<uint8_t> &values) {
  Eigen::Matrix<uint32_t, Eigen::Dynamic, 1> column(values.size());
  for (size_t i = 0; i != values.size(); ++i)
    column[i] = static_cast<uint32_t>(values[i]);
  File::Matrix::save_vector(column, path);
}

} // namespace MR::DWI::Tractography
