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

#include "min_max.h"
#include "progressbar.h"
#include "image/voxel.h"
#include "image/misc.h"

namespace MR {
  
  void get_min_max (Image::Voxel& ima, float& min, float& max) 
  {
    min = INFINITY;
    max = -INFINITY;

    ProgressBar::init (voxel_count(ima.image), "finding min/max...");

    do {
      float val = ima.real();
      if (finite (val)) {
        if (min > val) min = val;
        if (max < val) max = val;
      }
      if (ima.is_complex()) {
        val = ima.imag();
        if (finite (val)) {
          if (min > val) min = val;
          if (max < val) max = val;
        }
      }

      ProgressBar::inc();
    } while (ima++);

    ProgressBar::done();
  }

}

