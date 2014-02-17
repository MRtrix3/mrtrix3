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


#include "command.h"
#include "image/buffer.h"
#include "image/voxel.h"
#include "image/loop.h"
#include "progressbar.h"


using namespace MR;
using namespace App;

void usage () {
DESCRIPTION
  + "concatenate several images into one";

ARGUMENTS
  + Argument ("image1", "the first input image.")
  .type_image_in()

  + Argument ("image2", "the second input image.")
  .type_image_in()
  .allow_multiple()

  + Argument ("output", "the output image.")
  .type_image_out ();

OPTIONS
  + Option ("axis",
  "specify axis along which concatenation should be performed. By default, "
  "the program will use the axis after the last non-singleton axis of any "
  "of the input images")
  + Argument ("axis").type_integer (0, std::numeric_limits<int>::max(), std::numeric_limits<int>::max())

  + DataType::options();
}


typedef float value_type;


void run () {
  int axis = -1;

  Options opt = get_options ("axis");
  if (opt.size())
    axis = opt[0][0];

  int num_images = argument.size()-1;
  std::vector<Ptr<Image::Buffer<value_type> > > in (num_images);
  in[0] = new Image::Buffer<value_type> (argument[0]);
  Image::ConstHeader& header_in (*in[0]);

  int ndims = 0;
  int last_dim;

  for (int i = 1; i < num_images; i++) {
    in[i] = new Image::Buffer<value_type> (argument[i]);
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
          header_out.dim(i) = in[n]->dim (i);
          header_out.vox(i) = in[n]->vox (i);
          break;
        }
      }
    }
  }


  {
    size_t axis_dim = 0;
    for (int n = 0; n < num_images; n++) {
      if (in[n]->datatype().is_complex())
        header_out.datatype() = DataType::CFloat32;
      axis_dim += in[n]->ndim() > size_t (axis) ? (in[n]->dim (axis) > 1 ? in[n]->dim (axis) : 1) : 1;
    }
    header_out.dim(axis) = axis_dim;
  }

  header_out.datatype() = DataType::from_command_line (header_out.datatype());
  


  if (axis > 2) { // concatenate DW schemes
    size_t nrows = 0;
    for (int n = 0; n < num_images; ++n) {
      if (in[n]->DW_scheme().rows() == 0 ||  
          in[n]->DW_scheme().columns() != 4) {
        nrows = 0;
        break;
      }   
      nrows += in[n]->DW_scheme().rows();
    }   

    if (nrows) {
      header_out.DW_scheme().allocate (nrows, 4); 
      int row = 0;
      for (int n = 0; n < num_images; ++n) 
        for (size_t i = 0; i < in[n]->DW_scheme().rows(); ++i, ++row)
          for (size_t j = 0; j < 4; ++j) 
            header_out.DW_scheme()(row,j) = in[n]->DW_scheme()(i,j);
    }   
  }




  Image::Buffer<value_type> data_out (argument[num_images], header_out);
  Image::Buffer<value_type>::voxel_type out_vox (data_out);

  ProgressBar progress ("concatenating...", Image::voxel_count (out_vox));
  int axis_offset = 0;

  for (int i = 0; i < num_images; i++) {
    Image::Loop loop;
    Image::Buffer<value_type>::voxel_type in_vox (*in[i]);

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
