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


#include "dwi/tractography/properties.h"
#include "dwi/tractography/roi.h"
#include "adapter/subset.h"


namespace MR {
  namespace DWI {
    namespace Tractography {


    using namespace App;


    const OptionGroup ROIOption = OptionGroup ("Region Of Interest processing options")

      + Option ("include",
            "specify an inclusion region of interest, as either a binary mask image, "
            "or as a sphere using 4 comma-separared values (x,y,z,radius). Streamlines "
            "must traverse ALL inclusion regions to be accepted.")
          .allow_multiple()
          + Argument ("spec").type_text()

      + Option ("exclude",
            "specify an exclusion region of interest, as either a binary mask image, "
            "or as a sphere using 4 comma-separared values (x,y,z,radius). Streamlines "
            "that enter ANY exclude region will be discarded.")
          .allow_multiple()
          + Argument ("spec").type_text()

      + Option ("mask",
            "specify a masking region of interest, as either a binary mask image, "
            "or as a sphere using 4 comma-separared values (x,y,z,radius). If defined, "
            "streamlines exiting the mask will be truncated.")
          .allow_multiple()
          + Argument ("spec").type_text();



      void load_rois (Properties& properties)
      {
        auto opt = get_options ("include");
        for (size_t i = 0; i < opt.size(); ++i)
          properties.include.add (ROI (opt[i][0]));

        opt = get_options ("exclude");
        for (size_t i = 0; i < opt.size(); ++i)
          properties.exclude.add (ROI (opt[i][0]));

        opt = get_options ("mask");
        for (size_t i = 0; i < opt.size(); ++i)
          properties.mask.add (ROI (opt[i][0]));
      }






      Mask* get_mask (const std::string& name)
      {
        auto data = Image<bool>::open (name);
        std::vector<size_t> bottom (data.ndim(), 0), top (data.ndim(), 0);
        std::fill_n (bottom.begin(), 3, std::numeric_limits<size_t>::max());
        size_t sum = 0;

        for (auto l = Loop (0,3) (data); l; ++l) {
          if (data.value()) {
            ++sum;
            if (size_t(data.index(0)) < bottom[0]) bottom[0] = data.index(0);
            if (size_t(data.index(0)) > top[0])    top[0]    = data.index(0);
            if (size_t(data.index(1)) < bottom[1]) bottom[1] = data.index(1);
            if (size_t(data.index(1)) > top[1])    top[1]    = data.index(1);
            if (size_t(data.index(2)) < bottom[2]) bottom[2] = data.index(2);
            if (size_t(data.index(2)) > top[2])    top[2]    = data.index(2);
          } 
        }

        if (!sum)
          throw Exception ("Cannot use image " + name + " as ROI - image is empty");

        if (bottom[0]) --bottom[0];
        if (bottom[1]) --bottom[1];
        if (bottom[2]) --bottom[2];

        top[0] = std::min (size_t (data.size(0)-bottom[0]), top[0]+2-bottom[0]);
        top[1] = std::min (size_t (data.size(1)-bottom[1]), top[1]+2-bottom[1]);
        top[2] = std::min (size_t (data.size(2)-bottom[2]), top[2]+2-bottom[2]);

        Header new_info (data);
        for (size_t axis = 0; axis != 3; ++axis) {
          new_info.size(axis) = top[axis];
          for (size_t i = 0; i < 3; ++i)
            new_info.transform().translation()[3] += bottom[axis] * new_info.voxsize(axis) * new_info.transform().translation()[axis];
        }

        auto sub = Adapter::make<Adapter::Subset> (data, bottom, top);
        
        return new Mask (sub, new_info, data.name());
      }


    }
  }
}




