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
#include "dwi/sdeconv/constrained.h"

namespace MR
{
  namespace DWI
  {

    using namespace App;

    const OptionGroup CSD_options =
      OptionGroup ("Spherical deconvolution options")
      + Option ("lmax",
                "set the maximum harmonic order for the output series. By default, the "
                "program will use the highest possible lmax given the number of "
                "diffusion-weighted images, up to a maximum of 8.")
      + Argument ("order").type_integer (2, 30)

      + Option ("mask",
                "only perform computation within the specified binary brain mask image.")
      + Argument ("image").type_image_in()

      + Option ("directions",
                "specify the directions over which to apply the non-negativity constraint "
                "(by default, the built-in 300 direction set is used). These should be "
                "supplied as a text file containing the [ az el ] pairs for the directions.")
      + Argument ("file").type_file_in()

      + Option ("filter",
                "the linear frequency filtering parameters used for the initial linear "
                "spherical deconvolution step (default = [ 1 1 1 0 0 ]). These should be "
                " supplied as a text file containing the filtering coefficients for each "
                "even harmonic order.")
      + Argument ("spec").type_file_in()

      + Option ("neg_lambda",
                "the regularisation parameter lambda that controls the strength of the "
                "non-negativity constraint (default = " + str(DEFAULT_CSD_NEG_LAMBDA, 2) + ").")
      + Argument ("value").type_float (0.0)

      + Option ("norm_lambda",
                "the regularisation parameter lambda that controls the strength of the "
                "constraint on the norm of the solution (default = " + str(DEFAULT_CSD_NORM_LAMBDA, 2) + ").")
      + Argument ("value").type_float (0.0)

      + Option ("threshold",
                "the threshold below which the amplitude of the FOD is assumed to be zero, "
                "expressed as an absolute amplitude (default = " + str(DEFAULT_CSD_THRESHOLD, 2) + ").")
      + Argument ("value").type_float (-1.0, 10.0)

      + Option ("niter",
                "the maximum number of iterations to perform for each voxel (default = " + str(DEFAULT_CSD_NITER) + "). "
                "Use '-niter 0' for a linear unconstrained spherical deconvolution.")
      + Argument ("number").type_integer (0, 1000);


  }
}

