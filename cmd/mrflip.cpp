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
#include "dwi/gradient.h"
#include "image/buffer.h"
#include "image/voxel.h"
#include "image/loop.h"
#include "image/transform.h"



using namespace MR;
using namespace App;


void usage ()
{
  AUTHOR = "David Raffelt (d.raffelt@brain.org.au)";

  DESCRIPTION
  + "Flip an image across a given axis. If the input image is a DWI, then the gradient "
    "directions (defined wrt scanner coordinates) are also adjusted (wrt the chosen image axis)";

  ARGUMENTS
  + Argument ("input", "the input image").type_image_in()
  + Argument ("axis", "the axis to be flipped")
  + Argument ("output", "the output image").type_image_out();

  OPTIONS
  + DWI::GradOption;
}


void run ()
{
  Image::Header input_header (argument[0]);

  Math::Matrix<float> grad = DWI::get_DW_scheme<float> (input_header);
  int axis = argument[1];
  if (axis < 0 || axis > 2)
    throw Exception ("the image axis must be between 0 and 2 inclusive");

  if (grad.is_set()) {
    Image::Transform transform (input_header);
    for (size_t dir = 0; dir < grad.rows(); dir++) {
      Math::Vector<float> grad_flipped (3);
      transform.scanner2image_dir(grad.row(dir).sub(0,3), grad_flipped);
      grad_flipped[axis] = -grad_flipped[axis];
      Math::Vector<float> grad_flipped_scanner (3);
      transform.image2scanner_dir(grad_flipped, grad_flipped_scanner);
      grad.row(dir).sub(0,3) = grad_flipped_scanner;
    }
  }
  Image::Header output_header (input_header);
  if (grad.is_set()) {
    output_header.DW_scheme() = grad;
  }

 Image::Buffer<float> input_data (input_header);
 Image::Buffer<float>::voxel_type input_voxel(input_data);
 Image::Buffer<float> output_data (argument[2], output_header);
 Image::Buffer<float>::voxel_type output_voxel (output_data);

 Image::LoopInOrder loop (input_voxel, "flipping image...");
 for (loop.start (input_voxel); loop.ok(); loop.next(input_voxel)) {
   Image::voxel_assign(output_voxel, input_voxel);
   output_voxel[axis] = input_voxel.dim(axis) - input_voxel[axis] - 1;
   output_voxel.value() = input_voxel.value();
 }

}

