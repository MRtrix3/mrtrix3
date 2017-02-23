/* Copyright (c) 2008-2017 the MRtrix3 contributors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, you can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * MRtrix is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * For more details, see http://www.mrtrix.org/.
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

  AUTHOR = "J-Donald Tournier (jdtournier@gmail.com)";

  SYNOPSIS = "Estimate noise level voxel-wise using residuals from a truncated SH fit";
      
  DESCRIPTION
  + "WARNING: This command is deprecated and may be removed in future releases. "
    "Try using the dwidenoise command with the -noise option instead.";

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
            "diffusion-weighted images, up to a maximum of 8.")
  +   Argument ("order").type_integer (0, 30)

  + DWI::GradImportOptions()
  + DWI::ShellOption;

}


typedef float value_type;

void run ()
{
  WARN ("Command dwi2noise is deprecated. Try using dwidenoise with -noise option instead.");
  
  auto dwi_in = Image<value_type>::open (argument[0]);
  const auto grad = DWI::get_valid_DW_scheme (dwi_in);

  vector<size_t> dwis;
  Eigen::MatrixXd mapping;
  {
    dwis = DWI::Shells (grad).select_shells (true, false, true).largest().get_volumes();
    const auto dirs = DWI::gen_direction_matrix (grad, dwis);
    mapping = DWI::compute_SH2amp_mapping (dirs);
  }

  auto dwi = Adapter::make <Adapter::Extract1D> (dwi_in, 3, container_cast<vector<int>> (dwis));

  auto header = Header (dwi_in);
  header.ndim() = 3;
  header.datatype() = DataType::Float32;
  DWI::stash_DW_scheme (header, grad);
  auto noise = Image<value_type>::create (argument[1], header);

  DWI::estimate_noise (dwi, noise, mapping);
}


