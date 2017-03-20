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
#include "math/SH.h"
#include "dwi/gradient.h"
#include "dwi/shells.h"
#include "adapter/extract.h"
#include "dwi/svr/recon.h"


using namespace MR;
using namespace App;

void usage ()
{
  AUTHOR = "Daan Christiaens";

  SYNOPSIS = "Reconstruct DWI signal from a series of scattered slices with associated motion parameters.";

  DESCRIPTION
  + "";

  ARGUMENTS
  + Argument ("DWI", "the input DWI image.").type_image_in()
  + Argument ("SH", "the output spherical harmonics coefficients image.").type_image_out();


  OPTIONS
  + Option ("lmax",
            "set the maximum harmonic order for the output series.")
    + Argument ("order").type_integer(0, 30)

  + Option ("motion", 
            "the motion parameters associated with input slices or volumes.")
    + Argument ("file").type_file_in()

  + DWI::GradImportOptions()

  + DWI::ShellOption;

}


typedef float value_type;



void run ()
{
  auto dwi = Image<value_type>::open(argument[0]).with_direct_io(3);
  Header header (dwi);

  // force single-shell until multi-shell basis is implemented
  auto grad = DWI::get_valid_DW_scheme (dwi);
  DWI::Shells shells (grad);
  shells.select_shells (true, false, true);
  auto idx = shells.largest().get_volumes();
  auto dirs = DWI::gen_direction_matrix (grad, idx).template cast<float>();
  DWI::stash_DW_scheme (header, grad);
  
  // auto out = Image<value_type>::create (argument[1], header);

  // Select subset
  auto dwisub = Adapter::make <Adapter::Extract1D> (dwi, 3, container_cast<vector<int>> (idx));

  // Set up scattered data matrix
  DWI::ReconMatrix R (dwisub, dirs, 4);

  // Fit scattered data in basis...



}




