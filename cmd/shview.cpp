/*
 * Copyright (c) 2008-2016 the MRtrix3 contributors
 * 
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/
 * 
 * MRtrix is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * 
 * For more details, see www.mrtrix.org
 * 
 */


#include "gui/gui.h"
#include "command.h"
#include "progressbar.h"
#include "file/path.h"
#include "math/SH.h"
#include "gui/shview/icons.h"
#include "gui/shview/render_window.h"


using namespace MR;
using namespace App;

void usage ()
{
  AUTHOR = "J-Donald Tournier (jdtournier@gmail.com)";

  DESCRIPTION
  + "view spherical harmonics surface plots.";

  ARGUMENTS
  + Argument ("coefs",
              "a text file containing the even spherical harmonics coefficients to display.")
  .optional()
  .type_file_in();

  OPTIONS
  + Option ("response",
            "assume SH coefficients file only contains even, m=0 terms. Used to "
            "display the response function as produced by estimate_response");

  REQUIRES_AT_LEAST_ONE_ARGUMENT = false;
}





void run ()
{
  GUI::DWI::Window window (get_options ("response").size());

  if (argument.size())
    window.set_values (std::string (argument[0]));

  window.show();

  if (qApp->exec())
    throw Exception ("error running Qt application");
}

