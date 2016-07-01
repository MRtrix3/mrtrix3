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
#include "progressbar.h"
#include "image.h"
#include "dwi/gradient.h"
#include "algo/loop.h"
#include "adapter/extract.h"


using namespace MR;
using namespace App;
using namespace std;

typedef float value_type;

void usage ()
{

  AUTHOR = "David Raffelt (david.raffelt@florey.edu.au)";

  DESCRIPTION
    + "Extract either diffusion-weighted volumes or b=0 volumes from an image containing both";

  ARGUMENTS
    + Argument ("input", "the input DW image.").type_image_in ()
    + Argument ("output", "the output image (diffusion-weighted volumes by default.").type_image_out ();

  OPTIONS
    + Option ("bzero", "output b=0 volumes instead of the diffusion weighted volumes.")
    + DWI::GradImportOptions()
    + DWI::ShellOption
    + Stride::Options;
}

void run() 
{
  auto input_image = Image<float>::open (argument[0]);

  Eigen::MatrixXd grad = DWI::get_valid_DW_scheme (input_image);

  // Want to support non-shell-like data if it's just a straight extraction
  //   of all dwis or all bzeros i.e. don't initialise the Shells class
  std::vector<int> volumes;
  bool bzero = get_options ("bzero").size();
  auto opt = get_options ("shell");
  if (opt.size()) {
    DWI::Shells shells (grad);
    shells.select_shells (false, false);
    for (size_t s = 0; s != shells.count(); ++s) {
      DEBUG ("Including data from shell b=" + str(shells[s].get_mean()) + " +- " + str(shells[s].get_stdev()));
      for (const auto v : shells[s].get_volumes()) 
        volumes.push_back (v);
    }
    bzero = (shells.count() == 1 && shells[0].is_bzero());
  } else {
    const float bzero_threshold = File::Config::get_float ("BValueThreshold", 10.0);
    for (ssize_t row = 0; row != grad.rows(); ++row) {
      if ((bzero && (grad (row, 3) < bzero_threshold)) || (!bzero && (grad (row, 3) > bzero_threshold)))
        volumes.push_back (row);
    }
  }

  if (volumes.empty()) {
    auto type = (bzero) ? "b=0" : "dwi";
    throw Exception ("No " + str(type) + " volumes present");
  }

  std::sort (volumes.begin(), volumes.end());

  Header header (input_image);
  Stride::set_from_command_line (header);
  header.size (3) = volumes.size();

  Eigen::MatrixXd new_grad (volumes.size(), grad.cols());
  for (size_t i = 0; i < volumes.size(); i++)
    new_grad.row (i) = grad.row (volumes[i]);
  DWI::set_DW_scheme (header, new_grad);

  auto output_image = Image<float>::create (argument[1], header);

  auto input_volumes = Adapter::make<Adapter::Extract1D> (input_image, 3, volumes);
  threaded_copy_with_progress_message ("extracting volumes", input_volumes, output_image);
}
