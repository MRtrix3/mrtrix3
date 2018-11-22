/* Copyright (c) 2008-2019 the MRtrix3 contributors.
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
 * See the Mozila Public License v. 2.0 for more details.
 *
 * For more details, see http://www.mrtrix.org/.
 */

#include "command.h"
#include "image.h"
#include "algo/loop.h"

using namespace MR;
using namespace App;


void usage ()
{
  AUTHOR = "Robert E. Smith (robert.smith@florey.edu.au)";

  SYNOPSIS = "Print out the locations of all non-zero voxels in a mask image";

  DESCRIPTION
  + "If no destination file is specified, the voxel locations will be "
    "printed to stdout.";

  ARGUMENTS
  + Argument ("input",  "the input image.").type_image_in()
  + Argument ("output", "the (optional) output text file.").type_file_out().optional();
}


void run ()
{
  auto H = Header::open (argument[0]);
  if (H.datatype() != DataType::Bit)
    WARN ("Input is not a genuine boolean mask image");
  auto in = H.get_image<bool>();
  vector< Eigen::ArrayXi > locations;
  for (auto l = Loop(in) (in); l; ++l) {
    if (in.value()) {
      Eigen::ArrayXi this_voxel (in.ndim());
      for (size_t axis = 0; axis != in.ndim(); ++axis)
        this_voxel[axis] = in.index (axis);
      locations.push_back (std::move (this_voxel));
    }
  }
  Eigen::ArrayXXi prettyprint (locations.size(), in.ndim());
  for (size_t row = 0; row != locations.size(); ++row)
    prettyprint.row (row) = std::move (locations[row]);
  INFO ("Printing locations of " + str(prettyprint.rows()) + " non-zero voxels");
  if (argument.size() == 2)
    save_matrix (prettyprint, argument[1]);
  else
    std::cout << prettyprint;
}
