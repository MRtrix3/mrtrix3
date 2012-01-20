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
#include "image/subset.h"
#include "image/copy.h"


namespace MR {
  namespace DWI {
    namespace Tractography {



      void ROI::get_mask (Image::Header& mask_header) 
      {

        Image::Data<bool> mask_data (mask_header);
        Image::Data<bool>::voxel_type vox (mask_data);
        size_t bottom[] = { std::numeric_limits<size_t>::max(), std::numeric_limits<size_t>::max(), std::numeric_limits<size_t>::max() };
        size_t top[] = { 0, 0, 0 };
        size_t count = 0;

        Image::Loop loop (0,3);
        for (loop.start (vox); loop.ok(); loop.next (vox)) {
          if (vox.value()) {
            ++count;
            if (size_t(vox[0]) < bottom[0]) bottom[0] = vox[0];
            if (size_t(vox[0]) > top[0]) top[0] = vox[0];
            if (size_t(vox[1]) < bottom[1]) bottom[1] = vox[1];
            if (size_t(vox[1]) > top[1]) top[1] = vox[1];
            if (size_t(vox[2]) < bottom[2]) bottom[2] = vox[2];
            if (size_t(vox[2]) > top[2]) top[2] = vox[2];
          }
        }

        top[0] = top[0] + 1 - bottom[0];
        top[1] = top[1] + 1 - bottom[1];
        top[2] = top[2] + 1 - bottom[2];

        Image::Subset< Image::Data<bool>::voxel_type > sub (vox, 3, bottom, top);

        mask = new Mask (sub, mask_header.name());

        vol = vox.vox(0) * vox.vox(1) * vox.vox(2) * count;

      }


      void ROI::get_image (Image::Header& H)
      {

        Image::Data<float> data (H);
        Image::Data<float>::voxel_type vox (data);
        size_t bottom[] = { std::numeric_limits<size_t>::max(), std::numeric_limits<size_t>::max(), std::numeric_limits<size_t>::max() };
        size_t top[] = { 0, 0, 0 };
        float sum = 0.0, max = 0.0;

        Image::Loop loop (0,3);
        for (loop.start (vox); loop.ok(); loop.next (vox)) {
          if (vox.value() > 0.0) {
            sum += vox.value();
            max = MAX (max, vox.value());
            if (size_t(vox[0]) < bottom[0]) bottom[0] = vox[0];
            if (size_t(vox[0]) > top[0])    top[0]    = vox[0];
            if (size_t(vox[1]) < bottom[1]) bottom[1] = vox[1];
            if (size_t(vox[1]) > top[1])    top[1]    = vox[1];
            if (size_t(vox[2]) < bottom[2]) bottom[2] = vox[2];
            if (size_t(vox[2]) > top[2])    top[2]    = vox[2];
          } else if (vox.value() < 0.0) {
            throw Exception ("cannot have negative values in ROI");
          }
        }

        bottom[0] = bottom[0] ? bottom[0] - 1 : bottom[0];
        bottom[1] = bottom[1] ? bottom[1] - 1 : bottom[1];
        bottom[2] = bottom[2] ? bottom[2] - 1 : bottom[2];

        top[0] = MIN (size_t (H.dim (0) - 1), top[0] + 2 - bottom[0]);
        top[1] = MIN (size_t (H.dim (1) - 1), top[1] + 2 - bottom[1]);
        top[2] = MIN (size_t (H.dim (2) - 1), top[2] + 2 - bottom[2]);

        Image::Subset< Image::Data<float>::voxel_type > sub (vox, 3, bottom, top);

        image = new SeedImage (sub, H.name(), max);

        vol = vox.vox(0) * vox.vox(1) * vox.vox(2) * sum;

      }



    }
  }
}




