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

#define DEFAULT_NORM_VALUE 0.282094

void usage ()
{
  AUTHOR = "David Raffelt (david.raffelt@florey.edu.au)";

  DESCRIPTION
  + "Multi-tissue normalise. Globally normalise multiple tissue compartments (e.g. WM, GM, CSF) "
    "from multi-tissue CSD such that their sum (of their DC terms) within each voxel is as close to a scalar as possible "
    "(Default: sqrt(1/(4*pi)). Normalisation is performed by solving for a single scale factor to adjust each tissue type. \n\n"
    "Example usage: mtnormalise wm.mif wm_norm.mif gm.mif gm_norm.mif csf.mif csf_norm.mif";

  ARGUMENTS
  + Argument ("input output", "list of all input and output tissue compartment files. See example usage in the description. "
                              "Note that any number of tissues can be normalised").type_image_in().allow_multiple();

  OPTIONS
  + Option ("mask", "define the mask to compute the normalisation within. If not supplied this is estimated automatically")
    + Argument ("image").type_image_in ()
  + Option ("value", "specify the value to which the summed tissue compartments will be to (Default: sqrt(1/(4*pi) = " + str(DEFAULT_NORM_VALUE, 3) + ")")
    + Argument ("number").type_float ();
}


void run ()
{

  if (argument.size() % 2)
    throw Exception ("The number of input arguments must be even. There must be an output file provided for every input tissue image");

  vector<Image<float>> input_images;
  vector<Header> output_headers;
  vector<std::string> output_filenames;

  vector<size_t> sh_image_indexes;
  for (size_t i = 0; i < argument.size(); i += 2) {
    Header header = Header::open (argument[i]);
    if (header.ndim() == 4 && header.size(3) > 1) { // assume SH image to extract DC term
      auto dc = Adapter::make<Adapter::Extract1D> (header.get_image<float>(), 3, vector<int> (1, 0));
      input_images.emplace_back (Image<float>::scratch(dc));
      threaded_copy_with_progress_message ("loading image", dc, input_images[i / 2]);
      sh_image_indexes.push_back (i / 2);
    } else {
      input_images.emplace_back (Image<float>::open(argument[i]));
    }
    if (i)
      check_dimensions (input_images[0], input_images[i / 2], 0, 3);
    // we can't create the image yet if we want to put the scale factor into the output header
    output_filenames.push_back (argument[i + 1]);
    output_headers.emplace_back (header);
  }

  Image<bool> mask;
  auto opt = get_options("mask");
  if (opt.size()) {
    mask = Image<bool>::open (opt[0][0]);
  } else {
    auto summed = Image<float>::scratch (input_images[0], "summed image");
    for (size_t j = 0; j < input_images.size(); ++j) {
      for (auto i = Loop (summed, 0, 3) (summed, input_images[j]); i; ++i)
        summed.value() += input_images[j].value();
    }
    Filter::OptimalThreshold threshold_filter (summed);
    mask = Image<bool>::scratch (threshold_filter);
    threshold_filter (summed, mask);
  }
  size_t num_voxels = 0;
  for (auto i = Loop (mask) (mask); i; ++i) {
    if (mask.value())
      num_voxels++;
  }

  const float normalisation_value = get_option_value ("value", DEFAULT_NORM_VALUE);

  {
    ProgressBar progress ("normalising tissue compartments...");
    Eigen::MatrixXf X (num_voxels, input_images.size());
    Eigen::MatrixXf y (num_voxels, 1);
    y.fill (normalisation_value);
    uint32_t counter = 0;
    for (auto i = Loop (mask) (mask); i; ++i) {
      if (mask.value()) {
        for (size_t j = 0; j < input_images.size(); ++j) {
          assign_pos_of (mask, 0, 3).to (input_images[j]);
          X (counter, j) = input_images[j].value();
        }
        ++counter;
      }
    }
    progress++;
    Eigen::MatrixXf w = X.jacobiSvd (Eigen::ComputeThinU | Eigen::ComputeThinV).solve(y);
    progress++;
    for (size_t j = 0; j < input_images.size(); ++j) {
      float scale_factor = w(j,0);
      // If scale factor already present, we accumulate (for use in mtbin script)
      if (input_images[j].keyval().count("normalisation_scale_factor"))
        scale_factor *= std::stof(input_images[j].keyval().at("normalisation_scale_factor"));
      output_headers[j].keyval()["normalisation_scale_factor"] = str(scale_factor);
      auto output_image = Image<float>::create (output_filenames[j], output_headers[j]);
      if (std::find (sh_image_indexes.begin(), sh_image_indexes.end(), j) != sh_image_indexes.end()) {
        auto input = Image<float>::open (argument[j * 2]);
        for (auto i = Loop (input) (input, output_image); i; ++i)
          output_image.value() = input.value() * w(j,0);
      } else {
        for (auto i = Loop (input_images[j]) (input_images[j], output_image); i; ++i)
          output_image.value() = input_images[j].value() * w(j,0);
      }
      progress++;
    }
  }
}

