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

#include <filesystem>

#include "command.h"
#include "datatype.h"
#include <fmt/std.h>

#include "fixel/helpers.h"
#include "image.h"

#include "diff_images.h"

using namespace MR;
using namespace App;

// clang-format off
void usage() {

  AUTHOR = "David Raffelt (david.raffelt@florey.edu.au)"
           " and Robert E. Smith (robert.smith@florey.edu.au)";

  SYNOPSIS = "Compare two fixel images for differences, within specified tolerance";

  ARGUMENTS
  + Argument ("fixel1", "fixel directory.").type_directory_in()
  + Argument ("fixel2", "another fixel directory.").type_directory_in();

  OPTIONS
  + Testing::Diff_Image_Options;

}
// clang-format on

void run() {
  std::filesystem::path fixel_directory1{argument[0]};
  Fixel::check_fixel_directory(fixel_directory1);
  std::filesystem::path fixel_directory2{argument[1]};
  Fixel::check_fixel_directory(fixel_directory2);

  if (fixel_directory1 == fixel_directory2)
    throw Exception("Input fixel directories are the same");

  for (const auto &entry1 : std::filesystem::directory_iterator(fixel_directory1)) {
    const std::string fname = entry1.path().filename().string();
    auto in1 = Image<cdouble>::open(entry1.path());
    const std::filesystem::path filename2 = fixel_directory2 / fname;
    if (!std::filesystem::exists(filename2))
      throw Exception(fmt::format("File {} exists in fixel directory {} but not in fixel directory {}",
                                  fname,
                                  fixel_directory1,
                                  fixel_directory2));
    auto in2 = Image<cdouble>::open(filename2);
    Testing::diff_images(in1, in2);
  }
  for (const auto &entry2 : std::filesystem::directory_iterator(fixel_directory2)) {
    const std::string fname = entry2.path().filename().string();
    const std::filesystem::path filename1 = fixel_directory1 / fname;
    if (!std::filesystem::exists(filename1))
      throw Exception(fmt::format("File {} exists in fixel directory {} but not in fixel directory {}",
                                  fname,
                                  fixel_directory2,
                                  fixel_directory1));
  }
  CONSOLE("data checked OK");
}
