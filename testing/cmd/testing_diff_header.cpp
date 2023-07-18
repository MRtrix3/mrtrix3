/* Copyright (c) 2008-2023 the MRtrix3 contributors.
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

#include "command.h"
#include "diff_images.h"

using namespace MR;
using namespace App;

void usage ()
{
  AUTHOR = "Robert E. Smith (robert.smith@florey.edu.au)";
  SYNOPSIS = "Compare two images for differences in the basic contents of their headers";

  ARGUMENTS
  + Argument ("header1", "an image.").type_image_in()
  + Argument ("header2", "another image.").type_image_in();

  OPTIONS
  + Option ("keyval", "also test the contents of the key-value entries in the header");

}


void run ()
{
  auto in1 = Header::open (argument[0]);
  auto in2 = Header::open (argument[1]);

  check_headers (in1, in2);
  if (get_options ("keyval").size())
    check_keyvals (in1, in2);

  CONSOLE ("headers checked OK");
}

