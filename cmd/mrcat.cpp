/*
 * Copyright (c) 2008-2018 the MRtrix3 contributors.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, you can obtain one at http://mozilla.org/MPL/2.0/
 *
 * MRtrix3 is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * For more details, see http://www.mrtrix.org/
 */


#include "command.h"
#include "image.h"
#include "algo/loop.h"
#include "phase_encoding.h"
#include "progressbar.h"
#include "dwi/gradient.h"

using namespace MR;
using namespace App;

void usage ()
{
AUTHOR = "J-Donald Tournier (jdtournier@gmail.com) and Robert E. Smith (robert.smith@florey.edu.au)";

SYNOPSIS = "Concatenate several images into one";

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



template <typename value_type>
void write (vector<Header>& in,
            const size_t axis,
            Header& header_out,
            const std::string& out_path)
{
  auto image_out = Image<value_type>::create (out_path, header_out);
  size_t axis_offset = 0;

  for (size_t i = 0; i != in.size(); i++) {
    auto image_in = in[i].get_image<value_type>();

    auto copy_func = [&axis, &axis_offset](decltype(image_in)& in, decltype(image_out)& out)
    {
      out.index (axis) = axis < int(in.ndim()) ? in.index (axis) + axis_offset : axis_offset;
      out.value() = in.value();
    };

    ThreadedLoop ("concatenating \"" + image_in.name() + "\"", image_in, 0, std::min<size_t> (image_in.ndim(), image_out.ndim()))
      .run (copy_func, image_in, image_out);
    if (axis < int(image_in.ndim()))
      axis_offset += image_in.size (axis);
    else {
      ++axis_offset;
      image_out.index (axis) = axis_offset;
    }
  }
}



void run ()
{
  int axis = get_option_value ("axis", -1);

  int num_images = argument.size()-1;
  vector<Header> in (num_images);
  in[0] = Header::open (argument[0]);

  int ndims = 0;
  int last_dim;
  DataType datatype = in[0].datatype();
  DEBUG ("Datatype of first image (" + in[0].name() + "): " + datatype.specifier());

  for (int i = 1; i < num_images; i++) {
    in[i] = Header::open (argument[i]);
    for (last_dim = in[i].ndim()-1; in[i].size (last_dim) <= 1 && last_dim >= 0; last_dim--);
    if (last_dim > ndims)
      ndims = last_dim;
    DEBUG ("Datatype of image " + in[i].name() + ": " + in[i].datatype().specifier());
    if (in[i].datatype().is_complex())
      datatype.set_flag (DataType::Complex);
    if (datatype.is_integer() && in[i].datatype().is_signed())
      datatype.set_flag (DataType::Signed);
    if (in[i].datatype().is_floating_point() && datatype.is_integer())
      datatype = in[i].datatype();
    if (in[i].datatype().bytes() > datatype.bytes())
      datatype = (datatype() & DataType::Attributes) + (in[i].datatype()() & DataType::Type);
  }
  DEBUG (str("Output image datatype: ") + datatype.specifier());

  if (axis < 0) axis = std::max (3, ndims);
  ++ndims;

  for (int i = 0; i < ndims; i++)
    if (i != axis)
      for (int n = 0; n < num_images; n++)
        if (in[0].size (i) != in[n].size (i))
          throw Exception ("dimensions of input images do not match");

  if (axis >= ndims) ndims = axis+1;

  Header header_out (in[0]);
  header_out.datatype() = DataType::from_command_line (datatype);
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


  size_t axis_dim = 0;
  for (int n = 0; n < num_images; n++)
    axis_dim += in[n].ndim() > size_t (axis) ? (in[n].size (axis) > 1 ? in[n].size (axis) : 1) : 1;
  header_out.size (axis) = axis_dim;


  if (axis > 2) {
    // concatenate DW schemes
    ssize_t nrows = 0, ncols = 0;
    vector<Eigen::MatrixXd> input_grads;
    for (int n = 0; n < num_images; ++n) {
      auto grad = DWI::get_DW_scheme (in[n]);
      if (grad.rows() == 0 || grad.cols() < 4) {
        nrows = 0;
        break;
      }
      if (!ncols) {
        ncols = grad.cols();
      } else if (grad.cols() != ncols) {
        nrows = 0;
        break;
      }
      nrows += grad.rows();
      input_grads.push_back (std::move (grad));
    }
    if (nrows) {
      Eigen::MatrixXd grad_out (nrows, 4);
      int row = 0;
      for (int n = 0; n < num_images; ++n) {
        for (ssize_t i = 0; i < input_grads[n].rows(); ++i, ++row)
          grad_out.row(row) = input_grads[n].row(i);
      }
      DWI::set_DW_scheme (header_out, grad_out);
    } else {
      header_out.keyval().erase ("dw_scheme");
    }

    // concatenate PE schemes
    nrows = 0; ncols = 0;
    vector<Eigen::MatrixXd> input_schemes;
    for (int n = 0; n != num_images; ++n) {
      auto scheme = PhaseEncoding::parse_scheme (in[n]);
      if (!scheme.rows()) {
        nrows = 0;
        break;
      }
      if (!ncols) {
        ncols = scheme.cols();
      } else if (scheme.cols() != ncols) {
        nrows = 0;
        break;
      }
      nrows += scheme.rows();
      input_schemes.push_back (std::move (scheme));
    }
    Eigen::MatrixXd scheme_out;
    if (nrows) {
      scheme_out.resize (nrows, ncols);
      size_t row = 0;
      for (int n = 0; n != num_images; ++n)  {
        for (ssize_t i = 0; i != input_schemes[n].rows(); ++i, ++row)
          scheme_out.row(row) = input_schemes[n].row(i);
      }
    }
    PhaseEncoding::set_scheme (header_out, scheme_out);
  }


  const std::string out_path = argument[num_images];

  if (header_out.intensity_offset() == 0.0 && header_out.intensity_scale() == 1.0 && !datatype.is_floating_point()) {
    switch (datatype() & DataType::Type) {
      case DataType::Bit:
      case DataType::UInt8:
      case DataType::UInt16:
      case DataType::UInt32:
        if (datatype.is_signed())
          write<int32_t> (in, axis, header_out, out_path);
        else
          write<uint32_t> (in, axis, header_out, out_path);
        break;
      case DataType::UInt64:
        if (datatype.is_signed())
          write<int64_t> (in, axis, header_out, out_path);
        else
          write<uint64_t> (in, axis, header_out, out_path);
        break;
    }
  } else {
    if (datatype.is_complex())
      write<cdouble> (in, axis, header_out, out_path);
    else
      write<double> (in, axis, header_out, out_path);
  }

}

