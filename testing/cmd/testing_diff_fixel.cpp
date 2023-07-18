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


#include "image.h"
#include "fixel/helpers.h"

#include "diff_images.h"

using namespace MR;
using namespace App;

void usage ()
{
  AUTHOR = "David Raffelt (david.raffelt@florey.edu.au) and Robert E. Smith (robert.smith@florey.edu.au)";

  SYNOPSIS = "Compare two fixel images for differences, within specified tolerance";

  ARGUMENTS
  + Argument ("fixel1", "fixel directory.").type_directory_in()
  + Argument ("fixel2", "another fixel directory.").type_directory_in();

  OPTIONS
  + Testing::Diff_Image_Options;

}


void run ()
{
  std::string fixel_directory1 = argument[0];
  Fixel::check_fixel_directory (fixel_directory1);
  std::string fixel_directory2 = argument[1];
  Fixel::check_fixel_directory (fixel_directory2);

  if (fixel_directory1 == fixel_directory2)
    throw Exception ("Input fixel directorys are the same");

  auto dir_walker1 = Path::Dir (fixel_directory1);
  std::string fname;
  while ((fname = dir_walker1.read_name()).size()) {
    auto in1 = Image<cdouble>::open (Path::join (fixel_directory1, fname));
    std::string filename2 = Path::join (fixel_directory2, fname);
    if (!Path::exists (filename2))
      throw Exception ("File (" + fname + ") exists in fixel directory (" + fixel_directory1 + ") but not in fixel directory (" + fixel_directory2 + ") ");
    auto in2 = Image<cdouble>::open (filename2);
    Testing::diff_images (in1, in2);
  }
  auto dir_walker2 = Path::Dir (fixel_directory2);
  while ((fname = dir_walker2.read_name()).size()) {
    std::string filename1 = Path::join (fixel_directory1, fname);
    if (!Path::exists (filename1))
      throw Exception ("File (" + fname + ") exists in fixel directory (" + fixel_directory2 + ") but not in fixel directory (" + fixel_directory1 + ") ");
  }
  CONSOLE ("data checked OK");
}

