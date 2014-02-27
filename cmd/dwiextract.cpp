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
#include "image/voxel.h"
#include "dwi/gradient.h"
#include "image/loop.h"
#include "image/buffer.h"
#include "image/buffer_preload.h"


using namespace MR;
using namespace App;
using namespace std;

typedef float value_type;

void usage ()
{

  AUTHOR = "David Raffelt (d.raffelt@brain.org.au)";

  DESCRIPTION
    + "Extract either diffusion-weighted volumes or b=0 volumes from an image containing both";

  ARGUMENTS
    + Argument ("input", "the input DW image.").type_image_in ()
    + Argument ("output", "the output image (diffusion-weighted volumes by default.").type_image_out ();

  OPTIONS
    + Option ("bzero", "output b=0 volumes instead of the diffusion weighted volumes.")
    + DWI::GradOption
    + DWI::ShellOption;
}

void run() {
  std::vector<ssize_t> strides (4, 0);
  strides[3] = 1;
  Image::BufferPreload<float> data_in (argument[0], strides);
  Image::BufferPreload<float>::voxel_type voxel_in (data_in);

  Math::Matrix<value_type> grad (DWI::get_DW_scheme<float> (data_in));

  // Want to support non-shell-like data if it's just a straight extraction
  //   of all dwis or all bzeros i.e. don't initialise the Shells class
  std::vector<size_t> volumes;
  bool bzero = get_options ("bzero").size();
  Options opt = get_options ("shell");
  if (opt.size()) {
    DWI::Shells shells (grad);
    shells.select_shells (true, false);
    // Remove DW information from header if b=0 is the only 'shell' selected
    std::vector<int> bvalues = opt[0][0];
    bzero = (bvalues.size() == 1 && !bvalues[0]);
    for (size_t s = 0; s != shells.count(); ++s) {
      DEBUG ("Including data from shell b=" + str(shells[s].get_mean()) + " +- " + str(shells[s].get_stdev()));
      for (std::vector<size_t>::const_iterator v = shells[s].get_volumes().begin(); v != shells[s].get_volumes().end(); ++v)
        volumes.push_back (*v);
    }
  } else {
    const float bzero_threshold = File::Config::get_float ("BValueThreshold", 10.0);
    for (size_t row = 0; row != grad.rows(); ++row) {
      if ((bzero && (grad (row, 3) < bzero_threshold)) || (!bzero && (grad (row, 3) > bzero_threshold)))
        volumes.push_back (row);
    }
  }

  if (volumes.empty())
    throw Exception ("No " + str(bzero ? "b=0" : "dwi") + " volumes present");

  std::sort (volumes.begin(), volumes.end());

  Image::Header header (data_in);

  if (volumes.size() == 1)
    header.set_ndim (3);
  else
    header.dim (3) = volumes.size();

  if (bzero) {
    header.DW_scheme().clear();
  } else {
    Math::Matrix<value_type> new_grad (volumes.size(), 4);
    for (size_t i = 0; i < volumes.size(); i++)
      new_grad.row (i) = grad.row (volumes[i]);
    header.DW_scheme() = new_grad;
  }

  Image::Buffer<value_type> data_out (argument[1], header);
  Image::Buffer<value_type>::voxel_type voxel_out (data_out);

  Image::Loop outer ("extracting volumes...", 0, 3);

  if (voxel_out.ndim() == 4) {

    for (outer.start (voxel_out, voxel_in); outer.ok(); outer.next (voxel_out, voxel_in)) {
      for (size_t i = 0; i < volumes.size(); i++) {
        voxel_in[3] = volumes[i];
        voxel_out[3] = i;
        voxel_out.value() = voxel_in.value();
      }
    }

  } else {

    const size_t volume = volumes[0];
    for (outer.start (voxel_out, voxel_in); outer.ok(); outer.next (voxel_out, voxel_in)) {
      voxel_in[3] = volume;
      voxel_out.value() = voxel_in.value();
    }

  }


}
