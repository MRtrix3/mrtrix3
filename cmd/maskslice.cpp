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
#include "image.h"
#include "algo/threaded_copy.h"


using namespace MR;
using namespace App;

void usage ()
{
  AUTHOR = "Daan Christiaens";

  SYNOPSIS = "Select slice or multiband pack from image.";

  DESCRIPTION
  + "Reslice a mask or image according to a slice index and multiband factor.";

  ARGUMENTS
  + Argument ("in", "the input image.").type_image_in()
  + Argument ("s", "the slice index").type_integer(0)
  + Argument ("out", "the output image.").type_image_out();

  OPTIONS
  + Option ("mb", "the multiband factor. (default = 1)")
    + Argument ("order").type_integer(0, 32);

}


typedef float value_type;



void run ()
{
  auto in = Image<value_type>::open(argument[0]);

  // Read parameters
  size_t s  = argument[1];
  size_t mb = get_option_value("mb", 1);

  // Check dimensions
  if (in.ndim() != 3)
    throw Exception("Input image must be 3-dimensional.");
  if (s >= in.size(2))
    throw Exception("Slice index out of bounds.");
  if (in.size(2) % mb != 0)
    throw Exception("Multiband factor invalid.");

  // Write result to output file
  Header header (in);
  auto out = Image<value_type>::create (argument[2], header);

  size_t j = 0;
  size_t n = in.size(2) / mb;
  for (auto l = Loop(2)(in, out); l; l++, j++) {
    if (j % n == s % n) {
      threaded_copy(in, out, 0, 2);
    }
  }


}




