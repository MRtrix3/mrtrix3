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
#include "point.h"
#include "math/vector.h"
#include "image/buffer.h"
#include "image/voxel.h"
#include "image/loop.h"


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
  Image::Buffer<float> dir_buf (argument[0]);
  Image::Buffer<float>::voxel_type dir_vox (dir_buf);

  Image::Header header (argument[0]);
  header.dim(3) = header.dim(3)/3;

  Image::Buffer<float> amp_buf (argument[1], header);
  Image::Buffer<float>::voxel_type amp_vox (amp_buf);

  Image::LoopInOrder loop (dir_vox, "converting directions to amplitudes...", 0, 3);

  for (loop.start (dir_vox, amp_vox); loop.ok(); loop.next (dir_vox, amp_vox)) {
    Math::Vector<float> dir (3);
    dir_vox[3] = 0;
    amp_vox[3] = 0;
    while (dir_vox[3] < dir_vox.dim(3)) {
      dir[0] = dir_vox.value(); ++dir_vox[3];
      dir[1] = dir_vox.value(); ++dir_vox[3];
      dir[2] = dir_vox.value(); ++dir_vox[3];

      float amplitude = 0.0;
      if (isfinite (dir[0]) && isfinite (dir[1]) && isfinite (dir[2]))
        amplitude = Math::norm (dir);

      amp_vox.value() = amplitude;
      ++amp_vox[3];
    }
  }
}
