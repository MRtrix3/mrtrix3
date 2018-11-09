/*
 * Copyright (c) 2008-2018 the MRtrix3 contributors.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, you can obtain one at http://mozilla.org/MPL/2.0/
 *
 * MRtrix3 is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * For more details, see http://www.mrtrix.org/
 */


#include "dwi/sdeconv/csd.h"

namespace MR
{
  namespace DWI
  {
    namespace SDeconv
    {

    using namespace App;

    const OptionGroup CSD_options =
      OptionGroup ("Options for the Constrained Spherical Deconvolution algorithm")
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
                // TODO Explicit SD algorithm?
                "Use '-niter 0' for a linear unconstrained spherical deconvolution.")
      + Argument ("number").type_integer (0, 1000);


    }
  }
}

