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
#include "image/voxel.h"
#include "dataset/loop.h"
#include "progressbar.h"

using namespace std;
using namespace MR;

SET_VERSION_DEFAULT;
SET_AUTHOR (NULL);
SET_COPYRIGHT (NULL);

DESCRIPTION = {
  "concatenate several images into one",
  NULL
};

ARGUMENTS = {
  Argument ("image1", "first input image", "the first input image.").type_image_in (),
  Argument ("image2", "second input image", "the second input image.", AllowMultiple).type_image_in (),
  Argument ("output", "output image", "the output image.").type_image_out (),
  Argument::End
};

OPTIONS = {
  Option ("axis", "concatenation axis", "specify axis along which concatenation should be performed. By default, the program will use the axis after the last non-singleton axis of any of the input images")
    .append (Argument ("axis", "axis", "the concatenation axis").type_integer (0, INT_MAX, 0)),

  Option::End
};

typedef float value_type;


EXECUTE {
  int axis = -1;

  std::vector<OptBase> opt = get_options ("axis");
  if (opt.size()) axis = opt[0][0].get_int();

  int num_images = argument.size()-1;
  Ptr<Image::Header> in[num_images];
  Image::Header header (argument[0].get_image());

  int ndims = 0;
  int last_dim;

  for (int i = 0; i < num_images; i++) {
    in[i] = new Image::Header(argument[i].get_image());
    for (last_dim = in[i]->ndim()-1; in[i]->dim (last_dim) <= 1 && last_dim >= 0; last_dim--);
      if (last_dim > ndims)
        ndims = last_dim;
  }
  ndims++;

  if (axis < 0) axis = ndims-1;

  for (int i = 0; i < ndims; i++)
    if (i != axis)
      for (int n = 0; n < num_images; n++)
        if (in[0]->dim(i) != in[n]->dim(i))
          throw Exception ("dimensions of input images do not match");

  if (axis >= ndims) ndims = axis+1;

  header.axes.ndim() = ndims;

  for (uint i = 0; i < header.axes.ndim(); i++) {
    if (header.axes.dim(i) <= 1) {
      for (int n = 0; n < num_images; n++) {
        if (in[n]->ndim() > i) {
          header.axes[i] = (*in[n]).axes[i];
          break;
        }
      }
    }
  }

  header.axes.dim(axis) = 0;

  for (int n = 0; n < num_images; n++) {
    if (in[n]->is_complex())
      header.datatype() = DataType::CFloat32;

    if (static_cast<int>(in[n]->ndim()) > axis)
      header.axes.dim(axis) += in[n]->dim(axis) > 1 ? in[n]->dim(axis) : 1;
    else header.axes.dim(axis)++;
  }

  Image::Header output_header (argument[num_images].get_image (header));
  Image::Voxel<value_type> out_vox (output_header);
  ProgressBar progress("concatenating...", DataSet::voxel_count(out_vox));
  int axis_offset = 0;

  for (int i = 0; i < num_images; i++) {
    DataSet::Loop loop;
    Image::Voxel<value_type> in_vox(*in[i]);
    for (loop.start(in_vox); loop.ok(); loop.next(in_vox)) {
      for (uint dim = 0; dim < out_vox.ndim(); dim++) {
        if (static_cast<int>(dim) == axis)
          out_vox[dim] = dim < in_vox.ndim() ? axis_offset + in_vox[dim] : i;
        else
          out_vox[dim] = in_vox[dim];
      }
      out_vox.value() = in_vox.value();
      progress++;
    }
    if (axis < static_cast<int>(in_vox.ndim()))
      axis_offset += in_vox.dim(axis);
    else
      axis_offset++;
  }
}
