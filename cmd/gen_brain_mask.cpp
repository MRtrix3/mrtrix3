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

#include "app.h"
#include "point.h"
#include "image/data.h"
#include "image/scratch.h"
#include "image/voxel.h"
#include "filter/optimal_threshold.h"
#include "filter/median3D.h"
#include "ptr.h"
#include "image/histogram.h"
#include "image/copy.h"
#include "image/loop.h"
#include "dwi/gradient.h"

MRTRIX_APPLICATION

using namespace MR;
using namespace App;

void usage () {
  AUTHOR = "David Raffelt (draffelt@gmail.com)";

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


typedef float value_type;

void run () {
  Image::Header input_header (argument[0]);
  assert (!input_header.is_complex());
  Image::Data<value_type> input_data (input_header);
  Image::Data<value_type>::voxel_type input_voxel (input_data);

  Image::Header mask_header (input_header);
  mask_header.set_ndim(3);
  mask_header.create(argument[1]);
  Image::Data<float> mask_data (mask_header);
  Image::Data<float>::voxel_type mask_voxel (mask_data);

  std::vector<int> bzeros, dwis;
  Math::Matrix<float> grad = DWI::get_DW_scheme<float> (input_header);
  DWI::guess_DW_directions (dwis, bzeros, grad);
  info ("found " + str(dwis.size()) + " diffusion-weighted directions");

  Image::Header scratch_header;
  scratch_header = input_voxel;
  scratch_header.set_ndim (3);

  // Compute the mean b=0 and mean DWI image
  Image::Scratch<float> b0_mean_data (scratch_header, "mean b0");
  Image::Scratch<float>::voxel_type b0_mean (b0_mean_data);

  Image::Scratch<float> dwi_mean_data (scratch_header, "mean DWI");
  Image::Scratch<float>::voxel_type dwi_mean (dwi_mean_data);
  {
    Image::Loop loop("computing mean dwi and mean b0 images...", 0, 3);
    for (loop.start (input_voxel, b0_mean, dwi_mean); loop.ok(); loop.next (input_voxel, b0_mean, dwi_mean)) {
      float mean = 0;
      for (uint i = 0; i < dwis.size(); i++) {
        input_voxel[3] = dwis[i];
        mean += input_voxel.value();
      }
      dwi_mean.value() = mean / dwis.size();
      mean = 0;
      for (uint i = 0; i < bzeros.size(); i++) {
        input_voxel[3] = bzeros[i];
        mean += input_voxel.value();
      }
      b0_mean.value() = mean / bzeros.size();
    }
  }

  // Here we independently threshold the mean b=0 and dwi images
  Filter::OptimalThreshold<Image::Scratch<float>::voxel_type , Image::Scratch<int>::voxel_type > b0_threshold_filter (b0_mean);
  Image::Scratch<int> b0_mean_mask_data (b0_threshold_filter.get_output_params());
  Image::Scratch<int>::voxel_type b0_mean_mask (b0_mean_mask_data);
  b0_threshold_filter.execute (b0_mean_mask);

  Filter::OptimalThreshold<Image::Scratch<float>::voxel_type , Image::Scratch<float>::voxel_type > dwi_threshold_filter (dwi_mean);
  Image::Scratch<float> dwi_mean_mask_data (dwi_threshold_filter.get_output_params());
  Image::Scratch<float>::voxel_type dwi_mean_mask (dwi_mean_mask_data);
  dwi_threshold_filter.execute (dwi_mean_mask);

  {
    Image::Loop loop("combining optimal dwi and b0 masks...", 0, 3);
    for (loop.start (b0_mean_mask, dwi_mean_mask); loop.ok(); loop.next (b0_mean_mask, dwi_mean_mask)) {
      if (b0_mean_mask.value() > 0)
        dwi_mean_mask.value() = 1;
    }
  }

  Filter::Median3DFilter<Image::Scratch<float>::voxel_type, Image::Data<float>::voxel_type > median_filter (dwi_mean_mask);
  median_filter.execute (mask_voxel);
}

