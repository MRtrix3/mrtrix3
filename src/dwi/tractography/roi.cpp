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
#include "image/adapter/subset.h"
#include "image/copy.h"


namespace MR {
  namespace DWI {
    namespace Tractography {


      Mask* get_mask (const std::string& name)
      {
        Image::Buffer<bool> data (name);
        Image::Buffer<bool>::voxel_type vox (data);
        std::vector<size_t> bottom (vox.ndim(), 0), top (vox.ndim(), 0);
        std::fill_n (bottom.begin(), 3, std::numeric_limits<size_t>::max());
        size_t sum = 0;

        Image::Loop loop (0,3);
        for (loop.start (vox); loop.ok(); loop.next (vox)) {
          if (vox.value()) {
            ++sum;
            if (size_t(vox[0]) < bottom[0]) bottom[0] = vox[0];
            if (size_t(vox[0]) > top[0])    top[0]    = vox[0];
            if (size_t(vox[1]) < bottom[1]) bottom[1] = vox[1];
            if (size_t(vox[1]) > top[1])    top[1]    = vox[1];
            if (size_t(vox[2]) < bottom[2]) bottom[2] = vox[2];
            if (size_t(vox[2]) > top[2])    top[2]    = vox[2];
          } 
        }

        if (!sum)
          throw Exception ("Cannot use image " + name + " as ROI - image is empty");

        if (bottom[0]) --bottom[0];
        if (bottom[1]) --bottom[1];
        if (bottom[2]) --bottom[2];

        top[0] = std::min (size_t (data.dim(0)-bottom[0]), top[0]+2-bottom[0]);
        top[1] = std::min (size_t (data.dim(1)-bottom[1]), top[1]+2-bottom[1]);
        top[2] = std::min (size_t (data.dim(2)-bottom[2]), top[2]+2-bottom[2]);

        Image::Info new_info (data);
        for (size_t axis = 0; axis != 3; ++axis) {
          new_info.dim(axis) = top[axis];
          for (size_t i = 0; i < 3; ++i)
            new_info.transform()(i,3) += bottom[axis] * new_info.vox(axis) * new_info.transform()(i,axis);
        }

        Image::Adapter::Subset< Image::Buffer<bool>::voxel_type > sub (vox, bottom, top);
        
        return new Mask (sub, new_info, data.name());

      }


    }
  }
}




