/*
    Copyright 2008 Brain Research Institute, Melbourne, Australia

    Written by J-Donald Tournier 27/06/08 & David Raffelt 31/07/13,

    This file is part of MRtrix.

    MRtrix is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    MRtrix is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with MRtrix.  If not, see <http://www.gnu.org/licenses/>.

*/

#include "command.h"
#include "image.h"
#include "algo/loop.h"


using namespace std;
using namespace MR;
using namespace App;


void usage ()
{
  DESCRIPTION
  + "convert peak directions image to amplitudes.";

  ARGUMENTS
  + Argument ("directions", "the input directions image. Each volume corresponds to the x, y & z "
                             "component of each direction vector in turn.").type_image_in ()

  + Argument ("amplitudes", "the output amplitudes image.").type_image_out ();
}



void run ()
{
  auto dir = Image<float>::open (argument[0]);

  auto header = dir.original_header();
  header.size(3) = header.size(3)/3;

  auto amp = Image<float>::create (argument[1], header);
  
  auto loop = Loop("converting directions to amplitudes...", 0, 3);

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
