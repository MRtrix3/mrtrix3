/*
    Copyright 2011 Brain Research Institute, Melbourne, Australia

    Written by Robert E. Smith and J-Donald Tournier, 2011.

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

#include "image/filter/lcc.h"

#include "image/buffer.h"
#include "image/header.h"
#include "image/loop.h"
#include "image/voxel.h"





using namespace MR;
using namespace App;



void usage ()
{

	AUTHOR = "Robert E. Smith (r.smith@brain.org.au)";

  DESCRIPTION
  + "concatenate segmentation results from FSL FAST and FIRST into the 5TT format required for ACT";

  ARGUMENTS
  + Argument ("in_fast_one",   "first output PVE image from FAST (should be CSF)").type_image_in()
  + Argument ("in_fast_two",   "second output PVE image from FAST (should be GM)").type_image_in()
  + Argument ("in_fast_three", "third output PVE image from FAST (should be WM)").type_image_in()
  + Argument ("in_first",      "summed output images from FIRST containing subcortical grey matter structures").type_image_in()
  + Argument ("out",           "the output image path").type_image_out();

};



void run ()
{

  Image::Buffer<float> fast_csf (argument[0]);
  Image::Buffer<float> fast_gm  (argument[1]);
  Image::Buffer<float> fast_wm  (argument[2]);
  Image::Buffer<float> first    (argument[3]);

  for (size_t axis = 0; axis != 3; ++axis) {
    if (fast_csf.dim (axis) != fast_gm.dim (axis) || fast_gm.dim (axis) != fast_wm.dim (axis) || fast_wm.dim (axis) != first.dim (axis))
      throw Exception ("Input image dimensions must match!");
  }

  Image::Header H_out (fast_csf);
  H_out.set_ndim (4);
  H_out.dim(3) = 5;
  H_out.datatype() = DataType::Float32;
  Image::Stride::set (H_out, Image::Stride::contiguous_along_axis (3, fast_csf));
  Image::Buffer<float> out (argument[4], H_out);

  Image::Buffer<float>::voxel_type v_out      (out);
  Image::Buffer<float>::voxel_type v_fast_csf (fast_csf);
  Image::Buffer<float>::voxel_type v_fast_gm  (fast_gm);
  Image::Buffer<float>::voxel_type v_fast_wm  (fast_wm);
  Image::Buffer<float>::voxel_type v_first    (first);

  // Run LargestConnectedComponent on WM fraction to remove some BET / FAST errors
  Image::BufferScratch<float> wm_fixed (fast_wm);
  Image::BufferScratch<float>::voxel_type v_wm_fixed (wm_fixed);
  {
    Image::Filter::LargestConnectedComponent lcc (v_fast_wm);
    lcc.set_message ("cleaning up white matter fraction image...");
    lcc (v_fast_wm, v_wm_fixed);
  }

  Image::Loop loop ("concatenating images...", 0, 3);
  for (auto l = loop (v_out); l; ++l) {

    for (size_t axis = 0; axis != 3; ++axis)
      v_fast_csf[axis] = v_fast_gm[axis] = v_wm_fixed[axis] = v_first[axis] = v_out[axis];

    float csf = v_fast_csf.value();
    float cgm = v_fast_gm .value();
    float wm  = v_wm_fixed.value();
    float sgm = v_first   .value();

    // CSF is always preserved
    v_out[3] = 3; v_out.value() = csf;
    float sum = csf;

    // Sub-cortical grey matter masks grey and white matter, but do not exceed 1 if it overlaps with CSF
    sgm = std::min (sgm, float(1.0) - csf);
    v_out[3] = 1; v_out.value() = sgm;
    sum += sgm;

    // Normalise WM and CGM so sum is always 1.0
    const float gm_wm_multiplier = ((cgm + wm) <= std::numeric_limits<float>::epsilon()) ? 0.0 : ((1.0 - sum) / (cgm + wm));
    wm  *= gm_wm_multiplier;
    cgm *= gm_wm_multiplier;
    v_out[3] = 0; v_out.value() = cgm;
    v_out[3] = 2; v_out.value() = wm;

    // Pathological tissue is empty by default; can only be added using the 5ttedit command
    v_out[3] = 4; v_out.value() = 0.0;

  }

}
