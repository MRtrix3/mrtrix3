/*
    Copyright 2008 Brain Research Institute, Melbourne, Australia

    Written by J-Donald Tournier, 22/01/09.

    This file is part of MRtrix.

    MRtrix is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    MRtrix is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with MRtrix.  If not, see <http://www.gnu.org/licenses/>.

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

