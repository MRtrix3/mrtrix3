/*
   Copyright 2011 Brain Research Institute, Melbourne, Australia

   Written by David Raffelt, 2012.

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
#include "progressbar.h"
#include "image.h"
#include "dwi/gradient.h"
#include "algo/loop.h"


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
    + DWI::ShellOption;
}

void run()
{
  auto input_image = Image<float>::open (argument[0]).with_direct_io (Stride::contiguous_along_axis(3));
  Eigen::MatrixXd grad = DWI::get_valid_DW_scheme (input_image.header());

  // Want to support non-shell-like data if it's just a straight extraction
  //   of all dwis or all bzeros i.e. don't initialise the Shells class
  std::vector<size_t> volumes;
  bool bzero = get_options ("bzero").size();
  auto opt = get_options ("shell");
  if (opt.size()) {
    DWI::Shells shells (grad);
    shells.select_shells (false, false);
    for (size_t s = 0; s != shells.count(); ++s) {
      DEBUG ("Including data from shell b=" + str(shells[s].get_mean()) + " +- " + str(shells[s].get_stdev()));
      for (std::vector<size_t>::const_iterator v = shells[s].get_volumes().begin(); v != shells[s].get_volumes().end(); ++v)
        volumes.push_back (*v);
    }
    // Remove DW information from header if b=0 is the only 'shell' selected
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

  Header header (input_image.header());

  if (volumes.size() == 1)
    header.set_ndim (3);
  else
    header.size (3) = volumes.size();

  Eigen::MatrixXd new_grad (volumes.size(), grad.cols());
  for (size_t i = 0; i < volumes.size(); i++)
    new_grad.row (i) = grad.row (volumes[i]);
  header.set_DW_scheme (new_grad);

  auto output_image = Image<float>::create (argument[1], header);

  auto outer = Loop ("extracting volumes...", input_image, 0, 3);

  if (output_image.ndim() == 4) {
    for (auto i = outer (output_image, input_image); i; ++i) {
      for (size_t i = 0; i < volumes.size(); i++) {
        input_image.index(3) = volumes[i];
        output_image.index(3) = i;
        output_image.value() = input_image.value();
      }
    }

  } else {
    const size_t volume = volumes[0];
    for (auto i = outer (output_image, input_image); i; ++i) {
      input_image.index(3) = volume;
      output_image.value() = input_image.value();
    }
  }
}
