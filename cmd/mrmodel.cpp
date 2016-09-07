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
#include "transform.h"

using namespace MR;
using namespace App;



void usage ()
{
  AUTHOR = "David Raffelt (david.raffelt@florey.edu.au) & Rami Tabbara (rami.tabbara@florey.edu.au)";

  DESCRIPTION
  + "Model an input image using low frequency 3D polynomial basis functions. "
    "This command was designed to estimate a DWI bias field using the sum of normalised multi-tissue CSD compartments.";

  ARGUMENTS
  + Argument ("input", "the input image").type_image_in()
  + Argument ("output", "the output image representing the fit").type_image_out();


  OPTIONS
  + Option ("mask", "use only voxels within the supplied mask for the model fit. If not supplied this command will compute a mask")
    + Argument ("image").type_image_in ();
}


Eigen::VectorXf basis_function (Eigen::Vector3 pos) {
    float x = (float)pos[0];
    float y = (float)pos[1];
    float z = (float)pos[2];
    Eigen::VectorXf basis(19);
    basis(0) = 1.0;
    basis(1) = x;
    basis(2) = y;
    basis(3) = z;
    basis(4) = x * y;
    basis(5) = x * z;
    basis(6) = y * z;
    basis(7) = x * x;
    basis(8) = y * y;
    basis(9)= z * x;
    basis(10)= x * x * y;
    basis(11) = x * x * z;
    basis(12) = y * y * x;
    basis(13) = y * y * z;
    basis(14) = z * z * x;
    basis(15) = z * z * y;
    basis(16) = x * x * x;
    basis(17) = y * y * y;
    basis(18) = z * z * z;
    return basis;
}





void run ()
{
  auto input = Image<float>::open (argument[0]);
  auto output = Image<float>::create (argument[1], input);

  if (input.ndim() != 3)
    throw Exception ("input image must be 3D");

  Image<bool> mask;
  auto opt = get_options("mask");
  if (opt.size()) {
    mask = Image<bool>::open (opt[0][0]);
  } else {
    Filter::OptimalThreshold threshold_filter (input);
    mask = Image<bool>::scratch (threshold_filter);
    threshold_filter (input, mask);
  }

  size_t num_voxels = 0;
  for (auto i = Loop (mask) (mask); i; ++i) {
    if (mask.value())
      num_voxels++;
  }

  Eigen::MatrixXf X (num_voxels, 18);
  Eigen::VectorXf y (num_voxels);
  y.setOnes();


  {
    ProgressBar progress ("fitting model...");
    size_t counter = 0;
    Transform transform (input);
    for (auto i = Loop (mask) (input, mask); i; ++i) {
      if (mask.value()) {
        y [counter] = input.value();
        Eigen::Vector3 vox (input.index(0), input.index(1), input.index(2));
        Eigen::Vector3 pos = transform.voxel2scanner * vox;
        X.row (counter++) = basis_function (pos);
      }
    }
    progress++;

    Eigen::VectorXf w = X.jacobiSvd (Eigen::ComputeThinU | Eigen::ComputeThinV).solve(y);

    progress++;
    for (auto i = Loop (output) (output); i; ++i) {
      Eigen::Vector3 vox (output.index(0), output.index(1), output.index(2));
      Eigen::Vector3 pos = transform.voxel2scanner * vox;
      output.value() = basis_function (pos).dot (w);
    }
  }
}

