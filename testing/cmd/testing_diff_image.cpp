/* Copyright (c) 2008-2017 the MRtrix3 contributors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, you can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * MRtrix is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
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

  DESCRIPTION
  + "compare two images for differences, optionally with a specified tolerance.";

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

