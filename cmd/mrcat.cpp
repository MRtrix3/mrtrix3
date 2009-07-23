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
#include "progressbar.h"
#include "image/voxel.h"
#include "image/misc.h"

using namespace std; 
using namespace MR; 

SET_VERSION_DEFAULT;

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




EXECUTE {
  int axis = -1;

  std::vector<OptBase> opt = get_options (0); // axis
  if (opt.size()) axis = opt[0][0].get_int();

  int num_images = argument.size()-1;
  RefPtr<Image::Object> in[num_images];
  in[0] = argument[0].get_image();
  Image::Header header (in[0]->header());

  int ndims = 0;
  int last_dim;

  for (int i = 0; i < num_images; i++) {
    in[i] = argument[i].get_image();
    for (last_dim = in[i]->ndim()-1; in[i]->dim (last_dim) <= 1 && last_dim >= 0; last_dim--);
    if (last_dim > ndims) ndims = last_dim;
  }
  ndims++;

  if (axis < 0) axis = ndims-1;
  


  for (int i = 0; i < ndims; i++) 
    if (i != axis) 
      for (int n = 0; n < num_images; n++) 
        if (in[0]->dim(i) != in[n]->dim(i)) 
          throw Exception ("dimensions of input images do not match");


  if (axis >= ndims) ndims = axis+1;
  if (ndims > MRTRIX_MAX_NDIMS) 
    throw Exception ("maximum number of dimensions (" + str (MRTRIX_MAX_NDIMS) + ") exceeded");

  
  header.axes.resize (ndims);

  for (size_t i = 0; i < header.axes.size(); i++) {
    if (!header.axes[i].dim) {
      for (int n = 0; n < num_images; n++) {
        if (in[n]->ndim() > i) {
          header.axes[i] = in[n]->header().axes[i];
          break;
        }
      }
    }
  }


  header.axes[axis].dim = 0;
  header.data_type = DataType::Float32;
  for (int n = 0; n < num_images; n++) {
    if (int (in[n]->ndim()) > axis) 
      header.axes[axis].dim += in[n]->dim(axis) > 1 ? in[n]->dim(axis) : 1;
    else header.axes[axis].dim++;

    if (in[n]->is_complex()) 
      header.data_type = DataType::CFloat32;
  }

  Image::Voxel out (*argument[num_images].get_image (header));
  out.image.map();

  ProgressBar::init (voxel_count(out), "concatenating...");

  for (int i = 0; i < num_images; i++) {
    Image::Voxel pos (*in[i]);
    pos.image.map();

    do {
      float val = pos.real();
      out.real() = val;

      if (out.is_complex()) {
        val = pos.is_complex() ? pos.imag() : 0.0 ;
        out.imag() = val;
      }

      ProgressBar::inc();
      out++;
    } while (pos++);
  }

  ProgressBar::done();

    /*
    int lim = n[i].ndims() > axis ? in[i].dim(axis) : 1 ;
    for (uint a = 0; a < lim; a++) {

      Coord x(out);
      do {
        x[axis] = a;
        in[i].pos(x);

        x[axis] = count;
        out.pos(x);

        float val = in[i].re();
        out.re(val);

        if (out.is_complex()) {
          val = in[i].is_complex() ? in[i].im() : 0.0 ;
          out.im(val);
        }

        x[axis] = 0;
        ProgressBar::inc();

      } while (x.next(limits));

      count++;
    }

    in[i].unmap();
  }

*/
}

