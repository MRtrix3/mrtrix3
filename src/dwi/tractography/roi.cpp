/*******************************************************************************
    Copyright (C) 2014 Brain Research Institute, Melbourne, Australia
    
    Permission is hereby granted under the Patent Licence Agreement between
    the BRI and Siemens AG from July 3rd, 2012, to Siemens AG obtaining a
    copy of this software and associated documentation files (the
    "Software"), to deal in the Software without restriction, including
    without limitation the rights to possess, use, develop, manufacture,
    import, offer for sale, market, sell, lease or otherwise distribute
    Products, and to permit persons to whom the Software is furnished to do
    so, subject to the following conditions:
    
    The above copyright notice and this permission notice shall be included
    in all copies or substantial portions of the Software.
    
    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
    OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
    MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
    IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
    CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
    TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
    SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

*******************************************************************************/


#include "dwi/tractography/properties.h"
#include "dwi/tractography/roi.h"
#include "image/adapter/subset.h"
#include "image/copy.h"


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
          + Argument ("spec")

      + Option ("exclude",
            "specify an exclusion region of interest, as either a binary mask image, "
            "or as a sphere using 4 comma-separared values (x,y,z,radius). Streamlines "
            "that enter ANY exclude region will be discarded.")
          .allow_multiple()
          + Argument ("spec")

      + Option ("mask",
            "specify a masking region of interest, as either a binary mask image, "
            "or as a sphere using 4 comma-separared values (x,y,z,radius). If defined, "
            "streamlines exiting the mask will be truncated.")
          .allow_multiple()
          + Argument ("spec");



      void load_rois (Properties& properties)
      {
        Options opt = get_options ("include");
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




