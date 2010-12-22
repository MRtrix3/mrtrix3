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
  Argument ("image1", "the first input image.")
  .type_image_in(),

  Argument ("image2", "the second input image.")
  .type_image_in()
  .allow_multiple(),

  Argument ("output", "the output image.")
  .type_image_out (),

  Argument ()
};

OPTIONS = {

  Option ("axis",
  "specify axis along which concatenation should be performed. By default, "
  "the program will use the axis after the last non-singleton axis of any "
  "of the input images")
  + Argument ("axis").type_integer (0, std::numeric_limits<int>::max(), std::numeric_limits<int>::max()),

  Option ()
};

typedef float value_type;


EXECUTE {
  int axis = -1;

  Options opt = get_options ("axis");
  if (opt.size())
    axis = opt[0][0];

  int num_images = argument.size()-1;
  Ptr<Image::Header> in [num_images];
  in[0] = new Image::Header (argument[0]);
  Image::Header& header_in (*in[0]);

  int ndims = 0;
  int last_dim;

  for (int i = 1; i < num_images; i++) {
    in[i] = new Image::Header (argument[i]);
    for (last_dim = in[i]->ndim()-1; in[i]->dim (last_dim) <= 1 && last_dim >= 0; last_dim--);
    if (last_dim > ndims)
      ndims = last_dim;
  }
  ndims++;

  if (axis < 0) axis = ndims-1;

  for (int i = 0; i < ndims; i++)
    if (i != axis)
      for (int n = 0; n < num_images; n++)
        if (in[0]->dim (i) != in[n]->dim (i))
          throw Exception ("dimensions of input images do not match");

  if (axis >= ndims) ndims = axis+1;

  Image::Header header_out (header_in);
  header_out.set_ndim (ndims);

  for (size_t i = 0; i < header_out.ndim(); i++) {
    if (header_out.dim (i) <= 1) {
      for (int n = 0; n < num_images; n++) {
        if (in[n]->ndim() > i) {
          header_out.set_dim (i, in[n]->dim (i));
          header_out.set_vox (i, in[n]->vox (i));
          header_out.set_description (i, in[n]->description (i));
          header_out.set_units (i, in[n]->units (i));
          break;
        }
      }
    }
  }


  {
    size_t axis_dim = 0;
    for (int n = 0; n < num_images; n++) {
      if (in[n]->is_complex())
        header_out.set_datatype (DataType::CFloat32);
      axis_dim += in[n]->ndim() > size_t (axis) ? (in[n]->dim (axis) > 1 ? in[n]->dim (axis) : 1) : 1;
    }
    header_out.set_dim (axis, axis_dim);
  }

  header_out.create (argument[num_images]);
  Image::Voxel<value_type> out_vox (header_out);
  ProgressBar progress ("concatenating...", DataSet::voxel_count (out_vox));
  int axis_offset = 0;

  for (int i = 0; i < num_images; i++) {
    DataSet::Loop loop;
    Image::Voxel<value_type> in_vox (*in[i]);
    for (loop.start (in_vox); loop.ok(); loop.next (in_vox)) {
      for (size_t dim = 0; dim < out_vox.ndim(); dim++) {
        if (static_cast<int> (dim) == axis)
          out_vox[dim] = dim < in_vox.ndim() ? axis_offset + in_vox[dim] : i;
        else
          out_vox[dim] = in_vox[dim];
      }
      out_vox.value() = in_vox.value();
      progress++;
    }
    if (axis < static_cast<int> (in_vox.ndim()))
      axis_offset += in_vox.dim (axis);
    else
      axis_offset++;
  }
}
