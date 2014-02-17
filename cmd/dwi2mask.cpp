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
#include "point.h"
#include "ptr.h"
#include "image/buffer.h"
#include "image/buffer_preload.h"
#include "image/voxel.h"
#include "image/filter/dwi_brain_mask.h"



using namespace MR;
using namespace App;

void usage () {
  AUTHOR = "David Raffelt (d.raffelt@brain.org.au)";

DESCRIPTION
  + "Generates an whole brain mask from a DWI image."
    "Both diffusion weighted and b=0 volumes are required to "
    "obtain a mask that includes both brain tissue and CSF.";

ARGUMENTS
   + Argument ("image",
    "the input DWI image containing volumes that are both diffusion weighted and b=0")
    .type_image_in ()

   + Argument ("image",
    "the output whole brain mask image")
    .type_image_out ();

OPTIONS
  + DWI::GradOption;
}


void run () {
  std::vector<ssize_t> strides (4, 0);
  strides[3] = 1;
  Image::BufferPreload<float> input_data (argument[0], strides);
  Image::BufferPreload<float>::voxel_type input_voxel (input_data);

  Math::Matrix<float> grad = DWI::get_valid_DW_scheme<float> (input_data);

  Image::Filter::DWIBrainMask dwi_brain_mask_filter (input_voxel);

  Image::Header output_header (input_data);
  output_header.info() = dwi_brain_mask_filter.info();
  Image::Buffer<uint32_t> mask_data (argument[1], output_header);
  Image::Buffer<uint32_t>::voxel_type mask_voxel (mask_data);

  dwi_brain_mask_filter (input_voxel, grad, mask_voxel);
}
