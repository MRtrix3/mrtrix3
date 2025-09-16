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

// clang-format off
#include "gui.h"
#include "command.h"
// clang-format on

#include "file/path.h"
#include "math/SH.h"
#include "progressbar.h"
#include "shview/icons.h"
#include "shview/render_window.h"

using namespace MR;
using namespace App;

// clang-format off
void usage() {

  AUTHOR = "J-Donald Tournier (jdtournier@gmail.com)";

  SYNOPSIS = "View spherical harmonics surface plots";

  ARGUMENTS
  + Argument ("coefs", "a text file containing the even order"
                       " spherical harmonics coefficients to display.").optional().type_file_in();

  OPTIONS
  + Option ("response",
            "assume SH coefficients file only contains m=0 terms"
            " (zonal harmonics)."
            " Used to display the response function as produced by eg. amp2response");

  REQUIRES_AT_LEAST_ONE_ARGUMENT = false;

}
// clang-format on

void run() {
  GUI::DWI::Window window(!get_options("response").empty());

  if (!argument.empty())
    window.set_values(std::string(argument[0]));

  window.show();

  if (qApp->exec())
    throw Exception("error running Qt application");
}
