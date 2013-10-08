/*
    Copyright 2011 Brain Research Institute, Melbourne, Australia

    Written by Robert E. Smith, 2013.

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
#include "ptr.h"

#include "image/buffer.h"
#include "image/info.h"
#include "image/loop.h"
#include "image/nav.h"
#include "image/position.h"
#include "image/voxel.h"



MRTRIX_APPLICATION

using namespace MR;
using namespace App;



void usage ()
{

	AUTHOR = "Robert E. Smith (r.smith@brain.org.au)";

  DESCRIPTION
  + "manually set the partial volume fractions in a four-tissue-type (4TT) image using mask images";

  ARGUMENTS
  + Argument ("input",  "the 4TT image");

  OPTIONS
  + Option ("cgm", "provide a mask of voxels that should be set to cortical grey matter")
    + Argument ("image").type_image_in()

  + Option ("sgm", "provide a mask of voxels that should be set to sub-cortical grey matter")
    + Argument ("image").type_image_in()

  + Option ("wm",  "provide a mask of voxels that should be set to white matter")
    + Argument ("image").type_image_in()

  + Option ("csf", "provide a mask of voxels that should be set to CSF")
    + Argument ("image").type_image_in()

  + Option ("none", "provide a mask of voxels that should be cleared (i.e. are non-brain); note that this will supersede all other provided masks")
    + Argument ("image").type_image_in();

};




class Modifier
{
  public:
    Modifier (Image::Buffer<float>& image) :
      v (image) { }

    void set_cgm_mask  (const std::string& path) { load (path, 0); }
    void set_sgm_mask  (const std::string& path) { load (path, 1); }
    void set_wm_mask   (const std::string& path) { load (path, 2); }
    void set_csf_mask  (const std::string& path) { load (path, 3); }
    void set_none_mask (const std::string& path) { load (path, 4); }


    void set_values()
    {
      bool voxel_nulled = false;
      if (voxels[4]) {
        Image::Nav::set_pos (*voxels[4], *this, 0, 3);
        if (voxels[4]->value()) {
          for (v[3] = 0; v[3] != 4; ++v[3])
            v.value() = 0.0;
          voxel_nulled = true;
        }
      }
      if (!voxel_nulled) {
        unsigned int sum = 0;
        memset (values, 0, 4 * sizeof (float));
        for (size_t tissue = 0; tissue != 4; ++tissue) {
          if (voxels[tissue]) {
            Image::Nav::set_pos (*voxels[tissue], v, 0, 3);
            if (voxels[tissue]->value()) {
              ++sum;
              values[tissue] = 1.0;
            }
          }
        }
        if (sum) {
          if (sum > 1) {
            const float multiplier = 1.0 / float(sum);
            for (size_t tissue = 0; tissue != 4; ++tissue)
              values[tissue] *= multiplier;
          }
          for (v[3] = 0; v[3] != 4; ++v[3])
            v.value() = values[v[3]];
        }
      }
    }


    // Give transparency to the necessary members of v, such that this class can be used in Image::Loop
    ssize_t operator[] (size_t axis) const { return v[axis]; }
    Image::Position< Image::Buffer<float>::voxel_type > operator[] (size_t axis) { return Image::Position< Image::Buffer<float>::voxel_type > (v, axis); }
    size_t  ndim            ()            const { return v.ndim(); }
    ssize_t dim             (size_t axis) const { return v.dim (axis); }
    ssize_t stride          (size_t axis) const { return v.stride (axis); }
    const Image::Info& info ()            const { return v.info(); }


  private:
    Image::Buffer<float>::voxel_type v;
    Ptr< Image::Buffer<bool> > buffers[5];
    Ptr< Image::Buffer<bool>::voxel_type > voxels[5];
    float values[4];

    void load (const std::string& path, const size_t index)
    {
      assert (index <= 4);
      buffers[index] = new Image::Buffer<bool> (path);
      if (!Image::dimensions_match (v, *buffers[index], 0, 3))
        throw Exception ("Image " + str(path) + " does not match 4TT image dimensions");
      voxels[index] = new Image::Buffer<bool>::voxel_type (*buffers[index]);
    }

};




void run ()
{

  Image::Buffer<float> in (argument[0], true); // Open as read-write
  if (in.ndim() != 4 || in.dim (3) != 4)
    throw Exception ("Input image is not a four-tissue-type (4TT) segmented image");

  Modifier modifier (in);

  Options
  opt = get_options ("cgm");  if (opt.size()) modifier.set_cgm_mask  (opt[0][0]);
  opt = get_options ("sgm");  if (opt.size()) modifier.set_sgm_mask  (opt[0][0]);
  opt = get_options ("wm");   if (opt.size()) modifier.set_wm_mask   (opt[0][0]);
  opt = get_options ("csf");  if (opt.size()) modifier.set_csf_mask  (opt[0][0]);
  opt = get_options ("none"); if (opt.size()) modifier.set_none_mask (opt[0][0]);

  Image::LoopInOrder loop (modifier, 0, 3);
  for (loop.start (modifier); loop.ok(); loop.next (modifier)) {
    modifier.set_values();
  }

}

