/*
 * Copyright (c) 2008-2016 the MRtrix3 contributors
 * 
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/
 * 
 * MRtrix is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * 
 * For more details, see www.mrtrix.org
 * 
 */


#include "command.h"
#include "image.h"
#include "algo/loop.h"
#include "progressbar.h"
#include "dwi/gradient.h"

using namespace MR;
using namespace App;

void usage () {
AUTHOR = "J-Donald Tournier (jdtournier@gmail.com)";

DESCRIPTION
  + "concatenate several images into one";

ARGUMENTS
  + Argument ("image1", "the first input image.")
  .type_image_in()

  + Argument ("image2", "additional input image(s).")
  .type_image_in()
  .allow_multiple()

  + Argument ("output", "the output image.")
  .type_image_out ();

OPTIONS
  + Option ("axis",
  "specify axis along which concatenation should be performed. By default, "
  "the program will use the last non-singleton, non-spatial axis of any of "
  "the input images - in other words axis 3 or whichever axis (greater than 3) "
  "of the input images has size greater than one.")
  + Argument ("axis").type_integer (0)

  + DataType::options();
}


typedef float value_type;


void run () {
  int axis = get_option_value ("axis", -1);

  int num_images = argument.size()-1;
  std::vector<Header, Eigen::aligned_allocator<Header>> in (num_images);
  in[0] = Header::open (argument[0]);

  int ndims = 0;
  int last_dim;

  for (int i = 1; i < num_images; i++) {
    in[i] = Header::open (argument[i]);
    for (last_dim = in[i].ndim()-1; in[i].size (last_dim) <= 1 && last_dim >= 0; last_dim--);
    if (last_dim > ndims)
      ndims = last_dim;
  }

  if (axis < 0) axis = std::max (3, ndims);
  ++ndims;

  for (int i = 0; i < ndims; i++)
    if (i != axis)
      for (int n = 0; n < num_images; n++)
        if (in[0].size (i) != in[n].size (i))
          throw Exception ("dimensions of input images do not match");

  if (axis >= ndims) ndims = axis+1;

  Header header_out (in[0]);
  header_out.ndim() = ndims;

  for (size_t i = 0; i < header_out.ndim(); i++) {
    if (header_out.size (i) <= 1) {
      for (int n = 0; n < num_images; n++) {
        if (in[n].ndim() > i) {
          header_out.size(i) = in[n].size (i);
          header_out.spacing(i) = in[n].spacing (i);
          break;
        }
      }
    }
  }


  {
    size_t axis_dim = 0;
    for (int n = 0; n < num_images; n++) {
      if (in[n].datatype().is_complex())
        header_out.datatype() = DataType::CFloat32;
      axis_dim += in[n].ndim() > size_t (axis) ? (in[n].size (axis) > 1 ? in[n].size (axis) : 1) : 1;
    }
    header_out.size (axis) = axis_dim;
  }

  header_out.datatype() = DataType::from_command_line (header_out.datatype());
  


  if (axis > 2) { // concatenate DW schemes
    size_t nrows = 0;
    std::vector<Eigen::MatrixXd> input_grads;
    for (int n = 0; n < num_images; ++n) {
      auto grad = DWI::get_DW_scheme (in[n]);
      input_grads.push_back (grad);
      if (grad.rows() == 0 || grad.cols() < 4) {
        nrows = 0;
        break;
      }   
      nrows += grad.rows();
    }   

    if (nrows) {
      Eigen::MatrixXd grad_out (nrows, 4);
      int row = 0;
      for (int n = 0; n < num_images; ++n) 
        for (ssize_t i = 0; i < input_grads[n].rows(); ++i, ++row)
          for (size_t j = 0; j < 4; ++j) 
            grad_out (row,j) = input_grads[n](i,j);
      DWI::set_DW_scheme (header_out, grad_out);
    }
  }



  auto image_out = Image<value_type>::create (argument[num_images], header_out);
  int axis_offset = 0;


  for (int i = 0; i < num_images; i++) {
    auto image_in = in[i].get_image<value_type>();

    auto copy_func = [&axis, &axis_offset](decltype(image_in)& in, decltype(image_out)& out)
    {
      out.index (axis) = axis < int(in.ndim()) ? in.index (axis) + axis_offset : axis_offset;
      out.value() = in.value();
    };

    ThreadedLoop ("concatenating \"" + image_in.name() + "\"...", image_in, 0, std::min<size_t> (image_in.ndim(), image_out.ndim()))
      .run (copy_func, image_in, image_out);
    if (axis < int(image_in.ndim()))
      axis_offset += image_in.size (axis);
    else {
      ++axis_offset;
      image_out.index (axis) = axis_offset;
    }
  }
}

