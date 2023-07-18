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
#include "datatype.h"

#include "progressbar.h"
#include "image.h"
#include "diff_images.h"

using namespace MR;
using namespace App;

void usage ()
{
  AUTHOR = "J-Donald Tournier (jdtournier@gmail.com) and David Raffelt (david.raffelt@florey.edu.au) and Robert E. Smith (robert.smith@florey.edu.au)";

  SYNOPSIS = "Compare two images for differences, optionally with a specified tolerance";

  ARGUMENTS
  + Argument ("data1", "an image.").type_image_in()
  + Argument ("data2", "another image.").type_image_in();
  
  OPTIONS
  + Testing::Diff_Image_Options;

}


void run ()
{
  auto in1 = Image<cdouble>::open (argument[0]);
  auto in2 = Image<cdouble>::open (argument[1]);

  Testing::diff_images (in1, in2);

  CONSOLE ("data checked OK");
}

