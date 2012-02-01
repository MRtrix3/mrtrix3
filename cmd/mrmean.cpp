/*
    Copyright 2008 Brain Research Institute, Melbourne, Australia

    Written by J-Donald Tournier, 27/11/09.

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
#include "image/voxel.h"
#include "image/data.h"
#include "image/data_preload.h"
#include "image/loop.h"

MRTRIX_APPLICATION

using namespace MR;
using namespace App;

void usage ()
{
  DESCRIPTION
  + "average image intensities along specified axis.";

  ARGUMENTS
  + Argument ("input", "the input image.").type_image_in ()
  + Argument ("axis", "the axis along which to average.").type_integer (0)
  + Argument ("mean", "the output mean image.").type_image_out ();
}







void run ()
{
  size_t axis = argument[1];
  std::vector<ssize_t> strides (axis+1, 0);
  strides[axis] = 1;
  Image::DataPreload<float> data_in (argument[0], strides);

  Image::Header header_out (data_in);
  header_out.datatype() = DataType::Float32;
  if (axis == data_in.ndim() - 1)
    header_out.set_ndim (data_in.ndim()-1);
  else
    header_out.dim(axis) = 1;

  Image::Data<float> data_out (header_out, argument[2]);

  Image::DataPreload<float>::voxel_type in (data_in);
  Image::Data<float>::voxel_type out (data_out);

  Image::Loop inner (axis, axis+1);
  Image::LoopInOrder outer (header_out, "averaging...");

  float N = data_in.dim (axis);
  for (outer.start (out, in); outer.ok(); outer.next (out, in)) {
    float mean = 0.0;
    for (inner.start (in); inner.ok(); inner.next (in))
      mean += in.value();
    out.value() = mean / N;
  }
}

