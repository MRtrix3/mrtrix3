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
