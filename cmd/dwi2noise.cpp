/*
   Copyright 2008 Brain Research Institute, Melbourne, Australia

   Written by J-Donald Tournier, 27/06/08.

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
#include "image.h"
#include "adapter/extract.h"
#include "dwi/gradient.h"
#include "math/least_squares.h"
#include "math/SH.h"

#include "dwi/noise_estimator.h"

using namespace MR;
using namespace App;


void usage ()
{
  DESCRIPTION
  + "estimate noise level voxel-wise using residuals from a truncated SH fit";

  ARGUMENTS
  + Argument ("dwi",
              "the input diffusion-weighted image.")
  .type_image_in ()

  + Argument ("noise",
              "the output noise map")
  .type_image_out ();



  OPTIONS
  + Option ("lmax",
            "set the maximum harmonic order for the output series. By default, the "
            "program will use the highest possible lmax given the number of "
            "diffusion-weighted images up to lmax = 8.")
  +   Argument ("order").type_integer (0, 8, 8)

  + DWI::GradImportOptions()
  + DWI::ShellOption;

}


typedef float value_type;

void run ()
{
  auto dwi_in = Image<value_type>::open (argument[0]);

  auto H_out = dwi_in.header();
  H_out.set_ndim (3);
  H_out.datatype() = DataType::Float32;
  auto noise = Image<value_type>::create (argument[1], H_out);

  std::vector<size_t> dwis;
  Eigen::MatrixXd mapping;
  {
    auto grad = DWI::get_valid_DW_scheme (dwi_in.header());
    dwis = DWI::Shells (grad).select_shells (true, true).largest().get_volumes();
    auto dirs = DWI::gen_direction_matrix (grad, dwis);
    mapping = DWI::compute_SH2amp_mapping (dirs);
  }

  auto dwi = Adapter::make <Adapter::Extract1D> (dwi_in, 3, container_cast<std::vector<int>> (dwis));

  DWI::estimate_noise (dwi, noise, mapping);
}


