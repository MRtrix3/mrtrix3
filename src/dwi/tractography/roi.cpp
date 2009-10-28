/*
    Copyright 2009 Brain Research Institute, Melbourne, Australia

    Written by J-Donald Tournier, 22/10/09.

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


#include "dwi/tractography/roi.h"
#include "dataset/subset.h"
#include "dataset/copy.h"


namespace MR {
  namespace DWI {
    namespace Tractography {

      void ROI::get_mask (const Image::Header& mask_header) {
        Image::Voxel<bool> vox (mask_header);
        size_t bottom[] = { SIZE_MAX, SIZE_MAX, SIZE_MAX };
        size_t top[] = { 0, 0, 0 };
        size_t count = 0;

        for (vox.pos(2,0); vox.pos(2) < vox.dim(2); vox.move(2,1)) {
          for (vox.pos(1,0); vox.pos(1) < vox.dim(1); vox.move(1,1)) {
            for (vox.pos(0,0); vox.pos(0) < vox.dim(0); vox.move(0,1)) {
              if (vox.value()) {
                ++count;
                if (size_t(vox.pos(0)) < bottom[0]) bottom[0] = vox.pos(0);
                if (size_t(vox.pos(0)) > top[0]) top[0] = vox.pos(0);
                if (size_t(vox.pos(1)) < bottom[1]) bottom[1] = vox.pos(1);
                if (size_t(vox.pos(1)) > top[1]) top[1] = vox.pos(1);
                if (size_t(vox.pos(2)) < bottom[2]) bottom[2] = vox.pos(2);
                if (size_t(vox.pos(2)) > top[2]) top[2] = vox.pos(2);
              }
            }
          }
        }

        top[0] = top[0] + 1 - bottom[0];
        top[1] = top[1] + 1 - bottom[1];
        top[2] = top[2] + 1 - bottom[2];

        DataSet::Subset<Image::Voxel<bool>, 3> sub (vox, bottom, top);

        mask = new Mask (sub, mask_header.name());

        vol = vox.vox(0) * vox.vox(1) * vox.vox(2) * count;
      }

    }
  }
}




