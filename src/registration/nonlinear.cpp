/* Copyright (c) 2008-2024 the MRtrix3 contributors.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Covered Software is provided under this License on an "as is"
 * basis, without warranty of any kind, either expressed, implied, or
 * statutory, including, without limitation, warranties that the
 * Covered Software is free of defects, merchantable, fit for a
 * particular purpose or non-infringing.
 * See the Mozilla Public License v. 2.0 for more details.
 *
 * For more details, see http://www.mrtrix.org/.
 */

#include "registration/nonlinear.h"

namespace MR::Registration {

using namespace App;
// clang-format off
const OptionGroup nonlinear_options =
    OptionGroup("Non-linear registration options")

    + Option("nl_warp",
             "the non-linear warp output defined as two deformation fields,"
             " where warp1 can be used to transform image1->image2"
             " and warp2 to transform image2->image1."
             " The deformation fields also encapsulate any linear transformation"
             " estimated prior to non-linear registration.")
      + Argument("warp1").type_image_out()
      + Argument("warp2").type_image_out()

    + Option("nl_warp_full",
             "output all warps used during registration."
             " This saves four different warps that map each image to a midway space and their inverses"
             " in a single 5D image file."
             " The 4th image dimension indexes the x,y,z component of the deformation vector"
             " and the 5th dimension indexes the field in this order:"
             " image1->midway, midway->image1, image2->midway, midway->image2."
             " Where image1->midway defines the field that maps image1 onto the midway space using the reverse convention."
             " When linear registration is performed first,"
             " the estimated linear transform will be included in the comments of the image header,"
             " and therefore the entire linear and non-linear transform can be applied"
             " (in either direction)"
             " using this output warp file with mrtransform")
      + Argument("image").type_image_out()

    + Option("nl_init",
             "initialise the non-linear registration with the supplied warp image."
             " The supplied warp must be in the same format as output using the -nl_warp_full option"
             " (i.e. have 4 deformation fields with the linear transforms in the image header)")
      + Argument("image").type_image_in()

    + Option("nl_scale",
             "use a multi-resolution scheme"
             " by defining a scale factor for each level using comma separated values"
             " (Default: 0.25,0.5,1.0)")
      + Argument("factor").type_sequence_float()

    + Option("nl_niter",
             "the maximum number of iterations."
             " This can be specified either as a single number for all multi-resolution levels,"
             " or a single value for each level."
             " (Default: 50)")
      + Argument("num").type_sequence_int()

    + Option("nl_update_smooth",
             "regularise the gradient update field with Gaussian smoothing"
             " (standard deviation in voxel units;"
             " Default 2.0)")
      + Argument("stdev").type_float()

    + Option("nl_disp_smooth",
             "regularise the displacement field with Gaussian smoothing"
             " (standard deviation in voxel units;"
             " Default 1.0)")
      + Argument("stdev").type_float()

    + Option("nl_grad_step",
             "the gradient step size for non-linear registration"
             " (Default: 0.5)")
      + Argument("num").type_float(0.0001, 1.0)

    + Option("nl_lmax",
             "explicitly set the lmax to be used per scale factor in non-linear FOD registration."
             " By default, FOD registration will use lmax 0,2,4"
             " with default scale factors 0.25,0.5,1.0 respectively."
             " Note that no reorientation will be performed with lmax = 0.")
      + Argument("num").type_sequence_int()

    // + Option("cc", "use cc metric with radius")
    // + Argument ("radius").type_integer (1,100)

    + Option("diagnostics_image",
             "write intermediate images for diagnostics purposes")
      + Argument("path");
// clang-format on

} // namespace MR::Registration
