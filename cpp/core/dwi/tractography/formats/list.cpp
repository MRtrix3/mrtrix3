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

#include "dwi/tractography/formats/list.h"

#include "dwi/tractography/formats/tck.h"
#include "dwi/tractography/formats/tt.h"
#include "dwi/tractography/formats/vtk.h"
#include "dwi/tractography/formats/vtx.h"

namespace MR::DWI::Tractography::Formats {

TCK tck_handler;
TT tt_handler;
VTK vtk_handler;
VTX vtx_handler;

const Base *handlers[] = {&tck_handler, &tt_handler, &vtk_handler, &vtx_handler, nullptr};

const Base *get_handler(const std::filesystem::path &path) {
  for (const Base **format = handlers; *format != nullptr; ++format) {
    if ((*format)->handles(path))
      return *format;
  }
  return nullptr;
}

} // namespace MR::DWI::Tractography::Formats
