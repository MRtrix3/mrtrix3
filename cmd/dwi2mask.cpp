/*
    Copyright 2008 Brain Research Institute, Melbourne, Australia

    Written by David Raffelt, 24/06/11.

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

#include "command.h"
#include "point.h"
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
    "All diffusion weighted and b=0 volumes are used to "
    "obtain a mask that includes both brain tissue and CSF.";

ARGUMENTS
   + Argument ("image",
    "the input DWI image containing volumes that are both diffusion weighted and b=0")
    .type_image_in ()

   + Argument ("image",
    "the output whole brain mask image")
    .type_image_out ();

OPTIONS
  + DWI::GradImportOptions();
}


void run () {
  Image::BufferPreload<float> input_data (argument[0], Image::Stride::contiguous_along_axis (3));
  Image::BufferPreload<float>::voxel_type input_voxel (input_data);

  Math::Matrix<float> grad = DWI::get_valid_DW_scheme<float> (input_data);

  Image::Filter::DWIBrainMask dwi_brain_mask_filter (input_voxel, grad);
  dwi_brain_mask_filter.set_message ("computing dwi brain mask... ");

  Image::Header output_header (input_data);
  output_header.info() = dwi_brain_mask_filter.info();
  output_header.datatype() = DataType::Bit;
  Image::Buffer<bool> mask_data (argument[1], output_header);
  Image::Buffer<bool>::voxel_type mask_voxel (mask_data);

  dwi_brain_mask_filter (input_voxel, mask_voxel);
}
