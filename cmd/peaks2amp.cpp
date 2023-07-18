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
#include "image.h"
#include "algo/loop.h"
#include "fixel/helpers.h"


using namespace MR;
using namespace App;


void usage ()
{
  AUTHOR = "J-Donald Tournier (jdtournier@gmail.com)";

  SYNOPSIS = "Extract amplitudes from a peak directions image";

  ARGUMENTS
  + Argument ("directions", "the input directions image. Each volume corresponds to the x, y & z "
                             "component of each direction vector in turn.").type_image_in ()

  + Argument ("amplitudes", "the output amplitudes image.").type_image_out ();
}



void run ()
{
  Header H_in = Header::open (argument[0]);
  Peaks::check (H_in);
  auto dir = H_in.get_image<float>();

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
