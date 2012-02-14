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
#include "ptr.h"
#include "image/data.h"
#include "image/scratch.h"
#include "image/voxel.h"
#include "image/filter/optimal_threshold.h"
#include "image/filter/median3D.h"
#include "image/histogram.h"
#include "image/copy.h"
#include "image/loop.h"
#include "dwi/gradient.h"

MRTRIX_APPLICATION

using namespace MR;
using namespace App;
using namespace Image;

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
  Data<value_type> input_data (argument[0]);
  Data<value_type>::voxel_type input_voxel (input_data);

  std::vector<int> bzeros, dwis;
  Math::Matrix<float> grad = DWI::get_DW_scheme<float> (input_data);
  DWI::guess_DW_directions (dwis, bzeros, grad);

  Image::Info info (input_data);
  info.set_ndim (3);

  // Compute the mean b=0 and mean DWI image
  Scratch<float> b0_mean_data (info, "mean b0");
  Scratch<float>::voxel_type b0_mean_voxel (b0_mean_data);

  Scratch<float> dwi_mean_data (info, "mean DWI");
  Scratch<float>::voxel_type dwi_mean_voxel (dwi_mean_data);
  {
    Loop loop("computing mean dwi and mean b0 images...", 0, 3);
    for (loop.start (input_voxel, b0_mean_voxel, dwi_mean_voxel); loop.ok(); loop.next (input_voxel, b0_mean_voxel, dwi_mean_voxel)) {
      float mean = 0;
      for (uint i = 0; i < dwis.size(); i++) {
        input_voxel[3] = dwis[i];
        mean += input_voxel.value();
      }
      dwi_mean_voxel.value() = mean / dwis.size();
      mean = 0;
      for (uint i = 0; i < bzeros.size(); i++) {
        input_voxel[3] = bzeros[i];
        mean += input_voxel.value();
      }
      b0_mean_voxel.value() = mean / bzeros.size();
    }
  }

  // Here we independently threshold the mean b=0 and dwi images
  Filter::OptimalThreshold b0_threshold_filter (b0_mean_data);
  Scratch<int> b0_mean_mask_data (b0_threshold_filter);
  Scratch<int>::voxel_type b0_mean_mask_voxel (b0_mean_mask_data);
  b0_threshold_filter (b0_mean_voxel, b0_mean_mask_voxel);

  Filter::OptimalThreshold dwi_threshold_filter (dwi_mean_data);
  Scratch<float> dwi_mean_mask_data (dwi_threshold_filter);
  Scratch<float>::voxel_type dwi_mean_mask_voxel (dwi_mean_mask_data);
  dwi_threshold_filter (dwi_mean_voxel, dwi_mean_mask_voxel);

  {
    Loop loop("combining optimal dwi and b0 masks...", 0, 3);
    for (loop.start (b0_mean_voxel, b0_mean_mask_voxel, dwi_mean_mask_voxel); loop.ok(); loop.next (b0_mean_voxel, b0_mean_mask_voxel, dwi_mean_mask_voxel)) {
      if (b0_mean_mask_voxel.value() > 0)
        dwi_mean_mask_voxel.value() = 1;
    }
  }
  Filter::Median3DFilter median_filter (dwi_mean_mask_voxel);

  Header header (input_data);
  header.set_info(median_filter);
  header.datatype() = DataType::Int8;
  Data<int> mask_data (header, argument[1]);
  Data<int>::voxel_type mask_voxel (mask_data);
  median_filter(dwi_mean_mask_voxel, mask_voxel);

  //TODO use connected components to fill holes and remove non-brain tissue
}

