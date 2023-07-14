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
#include "progressbar.h"
#include "datatype.h"
#include "math/rng.h"

#include "image.h"
#include "algo/threaded_loop.h"

using namespace MR;
using namespace App;

void usage ()
{
  AUTHOR = "J-Donald Tournier (jdtournier@gmail.com)";

  SYNOPSIS = "Generate a test image of random numbers";

  ARGUMENTS
  + Argument ("size", "the dimensions of the test data.").type_sequence_int ()
  + Argument ("data", "the output image.").type_image_out ();

  OPTIONS
    + Stride::Options
    + DataType::options();
}


void run ()
{
  vector<int> dim = argument[0];

  Header header;

  header.ndim() = dim.size();
  for (size_t n = 0; n < dim.size(); ++n) {
    header.size(n) = dim[n];
    header.spacing(n) = 1.0f;
  }
  header.datatype() = DataType::from_command_line (DataType::Float32);
  Stride::set_from_command_line (header, Stride::contiguous_along_spatial_axes (header));

  auto image = Header::create (argument[1], header).get_image<float>();

  struct fill { 
    Math::RNG rng;
    std::normal_distribution<float> normal;
    void operator() (decltype(image)& v) { v.value() = normal(rng); }
  };
  ThreadedLoop ("generating random data...", image).run (fill(), image);
}

