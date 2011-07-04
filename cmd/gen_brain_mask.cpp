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
#include "image/voxel.h"
#include "filter/optimal_threshold.h"
#include "filter/median3D.h"
#include "ptr.h"
#include "dataset/histogram.h"
#include "dataset/buffer.h"
#include "dataset/copy.h"
#include "dataset/loop.h"
#include "dwi/gradient.h"

using namespace std;
using namespace MR;

SET_VERSION_DEFAULT;
SET_AUTHOR (NULL);
SET_COPYRIGHT (NULL);

DESCRIPTION = {
  "Generates an whole brain mask from a DWI image."
  "Both diffusion weighted and b=0 volumes are required to "
  "obtain a mask that includes both brain tissue and CSF.",
  NULL
};

ARGUMENTS = {

  Argument ("image",
    "the input DWI image containing volumes that are both diffusion weighted and b=0")
    .type_image_in (),

  Argument ("image",
    "the output whole brain mask image")
    .type_image_out (),

  Argument ()
};


OPTIONS = {

  Option ("grad",
  "specify the diffusion-weighted gradient scheme used in the acquisition. "
  "The program will normally attempt to use the encoding stored in the image "
  "header. This should be supplied as a 4xN text file with each line is in "
  "the format [ X Y Z b ], where [ X Y Z ] describe the direction of the "
  "applied gradient, and b gives the b-value in units (1000 s/mm^2).")
  + Argument ("encoding").type_file(),

  Option ()
};

typedef float value_type;

EXECUTE {
  Image::Header input_header (argument[0]);
  assert (!input_header.is_complex());
  Image::Voxel<value_type> input_voxel (input_header);

  Image::Header mask_header (input_header);
  mask_header.set_ndim(3);
  mask_header.create(argument[1]);
  Image::Voxel<float> mask_voxel (mask_header);

  Math::Matrix<value_type> grad;
  Options opt = get_options ("grad");
  if (opt.size())
    grad.load (opt[0][0]);
  else {
    if (!input_header.DW_scheme().is_set())
      throw Exception ("no diffusion encoding found in image \"" + input_header.name() + "\"");
    grad = input_header.DW_scheme();
  }
  std::vector<int> bzeros, dwis;
  DWI::guess_DW_directions (dwis, bzeros, grad);
  info ("found " + str(dwis.size()) + " diffusion-weighted directions");

  // Compute the mean b=0 and mean DWI image
  DataSet::Buffer<float> b0_mean(input_voxel, 3, "mean b0");
  DataSet::Buffer<float> dwi_mean(input_voxel, 3, "mean DWI");
  {
    DataSet::Loop loop("computing mean dwi and mean b0 images...", 0, 3);
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
  Filter::OptimalThreshold<DataSet::Buffer<float> , DataSet::Buffer<int> > b0_threshold_filter(b0_mean);
  DataSet::Buffer<int> b0_mean_mask(b0_threshold_filter.get_output_params());
  b0_threshold_filter.execute(b0_mean_mask);

  Filter::OptimalThreshold<DataSet::Buffer<float> , DataSet::Buffer<float> > dwi_threshold_filter(dwi_mean);
  DataSet::Buffer<float> dwi_mean_mask(dwi_threshold_filter.get_output_params());
  dwi_threshold_filter.execute(dwi_mean_mask);

  {
    DataSet::Loop loop("combining optimal dwi and b0 masks...", 0, 3);
    for (loop.start (b0_mean_mask, dwi_mean_mask); loop.ok(); loop.next (b0_mean_mask, dwi_mean_mask)) {
      if (b0_mean_mask.value() > 0)
        dwi_mean_mask.value() = 1;
    }
  }

  Filter::Median3DFilter<DataSet::Buffer<float>, Image::Voxel<float> > median_filter(dwi_mean_mask);
  median_filter.execute(mask_voxel);
}

