/*
    Copyright 2008 Brain Research Institute, Melbourne, Australia

    Written by J-Donald Tournier, 27/06/08.

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

#include "app.h"
#include "progressbar.h"
#include "point.h"
#include "image/voxel.h"

using namespace std; 
using namespace MR; 

SET_VERSION_DEFAULT;

DESCRIPTION = {
  "convert directions image to amplitudes.",
  NULL
};

ARGUMENTS = {
  Argument ("directions", "directions image", "the input directions image. Each volume corresponds to the x, y & z component of each direction vector in turn.").type_image_in (),
  Argument ("amplitudes", "amplitudes image", "the output amplitudes image.").type_image_out (),
  Argument::End
};

OPTIONS = { Option::End };


EXECUTE {
  Image::Object &dir_obj (*argument[0].get_image());
  Image::Header header (dir_obj);

  header.data_type = DataType::Float32;
  header.axes.resize (4);
  header.axes[3].dim = dir_obj.dim(3)/3;

  Image::Voxel dir (dir_obj);
  Image::Voxel amp (*argument[1].get_image (header));

  ProgressBar::init (dir.dim(0)*dir.dim(1)*dir.dim(2), "converting orientations to amplitudes...");

  for (dir[2] = amp[2] = 0; dir[2] < dir.dim(2); dir[2]++, amp[2]++) {
    for (dir[1] = amp[1] = 0; dir[1] < dir.dim(1); dir[1]++, amp[1]++) {
      for (dir[0] = amp[0] = 0; dir[0] < dir.dim(0); dir[0]++, amp[0]++) {

        dir[3] = 0;
        amp[3] = 0;

        while (dir[3] < dir.dim(3)) {
          Point p;
          p[0] = dir.value(); dir[3]++;
          p[1] = dir.value(); dir[3]++;
          p[2] = dir.value(); dir[3]++;

          float amplitude = NAN;
          if (finite (p[0]) && finite (p[1]) && finite (p[2]) && p[0] != 0.0 && p[1] != 0.0 && p[2] != 0.0) 
            amplitude = p.norm();

          amp.value() = amplitude;
          amp[3]++;
        }

        ProgressBar::inc();
      }
    }
  }
  ProgressBar::done();
}


