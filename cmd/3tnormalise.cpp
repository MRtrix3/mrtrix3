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
#include "adapter/extract.h"
#include "filter/optimal_threshold.h"

using namespace MR;
using namespace App;



void usage ()
{
  AUTHOR = "David Raffelt (david.raffelt@florey.edu.au)";

  DESCRIPTION
  + "Globally normalise three tissue (wm, gm, csf) comparments from multi-tissue CSD "
    "such that their sum within each voxel is as close to 1 as possible. "
    "This involves solving for a single scale factor for each compartment map.";

  ARGUMENTS
  + Argument ("wm", "the white matter compartment (FOD) image").type_image_in()
  + Argument ("gm", "the grey matter compartment image").type_image_in()
  + Argument ("csf", "the cerebral spinal fluid comparment image").type_image_in()
  + Argument ("wm_out", "the output white matter compartment (FOD) image").type_image_out()
  + Argument ("gm_out", "the output grey matter compartment image").type_image_out()
  + Argument ("csf_out", "the output cerebral spinal fluid comparment image").type_image_out();

  OPTIONS
  + Option ("mask", "define the mask to compute the normalisation within. If not supplied this is estimated automatically")
    + Argument ("image").type_image_in ();
}


void run ()
{
  auto wm = Image<float>::open (argument[0]);
  auto gm = Image<float>::open (argument[1]);
  auto csf = Image<float>::open (argument[2]);

  check_dimensions (wm, gm, 0, 3);
  check_dimensions (csf, gm);

  auto wm_out = Image<float>::create (argument[3], wm);
  auto gm_out = Image<float>::create (argument[4], gm);
  auto csf_out = Image<float>::create (argument[5], csf);

  auto wm_dc = Adapter::make<Adapter::Extract1D> (wm, 3, std::vector<int> (1, 0));

  Image<bool> mask;
  auto opt = get_options("mask");
  if (opt.size()) {
    mask = Image<bool>::open (opt[0][0]);
  } else {
    auto summed = Image<float>::scratch (gm);
    for (auto i = Loop (gm) (gm, csf, wm_dc, summed); i; ++i)
      summed.value() = gm.value() + csf.value() + wm_dc.value();
    Filter::OptimalThreshold threshold_filter (summed);
    mask = Image<bool>::scratch (threshold_filter);
    threshold_filter (summed, mask);
  }
  size_t num_voxels = 0;
  for (auto i = Loop (mask) (mask); i; ++i) {
    if (mask.value())
      num_voxels++;
  }

  {
    ProgressBar progress ("normalising tissue compartments...");
    Eigen::MatrixXf X (num_voxels, 3);
    Eigen::MatrixXf y (num_voxels, 1);
    y.setOnes();
    size_t counter = 0;
    for (auto i = Loop (gm) (gm, csf, wm_dc, mask); i; ++i) {
      if (mask.value()) {
        X (counter, 0) = gm.value();
        X (counter, 1) = csf.value();
        X (counter++, 2) = wm_dc.value();
      }
    }
    progress++;
    Eigen::MatrixXf w = X.jacobiSvd (Eigen::ComputeThinU | Eigen::ComputeThinV).solve(y);
    progress++;
    for (auto i = Loop (gm) (gm, csf, gm_out, csf_out); i; ++i) {
      gm_out.value() = gm.value() * w(0,0);
      csf_out.value() = csf.value() * w(1,0);
    }
    progress++;
    for (auto i = Loop (wm) (wm, wm_out); i; ++i)
      wm_out.value() = wm.value() * w(2,0);
  }
}

