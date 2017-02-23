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
#include "__mrtrix_plugin.h"

#include "command.h"
#include "image.h"
#include "algo/loop.h"


using namespace std;
using namespace MR;
using namespace App;


void usage ()
{
  AUTHOR = "J-Donald Tournier (jdtournier@gmail.com)";

  SYNOPSIS = "Convert peak directions image to amplitudes";

  ARGUMENTS
  + Argument ("directions", "the input directions image. Each volume corresponds to the x, y & z "
                             "component of each direction vector in turn.").type_image_in ()

  + Argument ("amplitudes", "the output amplitudes image.").type_image_out ();
}



void run ()
{
  auto dir = Image<float>::open (argument[0]);

  Header header (dir);
  header.size(3) = header.size(3)/3;

  auto amp = Image<float>::create (argument[1], header);
  
  auto loop = Loop("converting directions to amplitudes", 0, 3);

  for (auto i = loop (dir, amp); i; ++i) {
    Eigen::Vector3f n;
    dir.index(3) = 0;
    amp.index(3) = 0;
    while (dir.index(3) < dir.size(3)) {
      n[0] = dir.value(); ++dir.index(3);
      n[1] = dir.value(); ++dir.index(3);
      n[2] = dir.value(); ++dir.index(3);

      float amplitude = 0.0;
      if (std::isfinite (n[0]) && std::isfinite (n[1]) && std::isfinite (n[2]))
        amplitude = n.norm();

      amp.value() = amplitude;
      ++amp.index(3);
    }
  }
}
